#ifndef CXXNET_LAYER_INL_HPP
#define CXXNET_LAYER_INL_HPP
#pragma once
/*!
 * \file cxxnet_layer-inl.hpp
 * \brief implementation of different layers
 * \author Bing Xu, Tianqi Chen
 */

#include "cxxnet_core.h"
#include "cxxnet_op.h"
#include "mshadow/tensor_container.h"
#include "mshadow/tensor.h"

//#include <ctime>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>

// DPP-specific includes
#include <DPP.h>
#include <Matrix.h>
#include <fstream>

// strata-specific includes
#include <vector>
#include <kmeans.h>
#include <fstream>

// simbased-specific includes
#include <math.h>

#if CXXNET_ADAPT_CAFFE
#include "../plugin/cxxnet_caffe_adapter-inl.hpp"
#endif
#include "cxxnet_pairtest-inl.hpp"

namespace cxxnet{
    template<typename xpu>
    inline ILayer* CreateLayer_( int type, mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out );
};

namespace cxxnet{
    // expr is needed to use expression
    using namespace mshadow::expr;
    using namespace mshadow::utils;
    // Random init method
    using namespace cxxnet::rnd_type;
    /*! \brief potential parameters for each layer */
    struct LayerParam{
        /*! \brief number of hidden layers */
        int num_hidden;
        /*! \brief initialization sd for weight */
        float init_sigma;
        /*! \brief intialization value for bias */
        float init_bias;
        /*! \brief initialization random type */
        int random_type;
        /*! \brief number of output channel */
        int num_channel;
        /*! \brief number of parallel group */
        int num_group;
        /*! \brief kernel size */
        int kernel_size;
        /*! \brief stride prameter */
        int stride;
        /*! \brief padding */
        int pad;
        /*! \brief whether not include bias term */
        int no_bias;
        /*! \brief dropout threshold  */
        float dropout_threshold;
        /*! \brief maximum temp_col_size allowed in each layer, we need at least one temp col */
        int temp_col_max;
        /*! \brief shut up */
        int silent;
        /*! \brief construtor */
        LayerParam( void ){
            init_sigma = 0.01f;
            init_bias  = 0.0f;
            num_hidden = 0;
            random_type = 0;
            num_channel = 0;
            num_group = 1;
            kernel_size = 0;
            stride = 1;
            pad = 0;
            dropout_threshold = 0.0f;
            no_bias = 0;
            silent = 0;
            // 64 MB
            temp_col_max = 64<<18;
        }
        /*!
         * \brief Set param for the layer from string
         * \param name parameter name
         * \param val string for configuration
         */
        inline void SetParam(const char *name, const char* val) {
            if( !strcmp( name, "init_sigma") )  init_sigma = (float)atof(val);
            if( !strcmp( name, "init_bias") )   init_bias  = (float)atof(val);
            if( !strcmp( name, "nhidden") )     num_hidden = atoi(val);
            if( !strcmp( name, "random_type") && !strcmp(val, "gaussian"))  random_type = 0;
            if( !strcmp( name, "random_type") && !strcmp(val, "xavier")) random_type = 1;
            if( !strcmp( name, "nchannel") )    num_channel = atoi(val);
            if( !strcmp( name, "ngroup") )      num_group = atoi(val);
            if( !strcmp( name, "kernel_size") ) kernel_size = atoi(val);
            if( !strcmp( name, "stride") )      stride      = atoi(val);
            if( !strcmp( name, "pad") )         pad      = atoi(val);
            if( !strcmp( name, "no_bias") )     no_bias = atoi(val);
            if( !strcmp( name, "threshold"))    dropout_threshold = (float)atof(val);
            if( !strcmp( name, "silent") )      silent   = atoi(val);
            if( !strcmp( name, "temp_col_max") ) temp_col_max = atoi(val) << 18;
        }
    };
};

namespace cxxnet {
    /*! \brief some common edge operations, so far it is initlaization, maybe move as common functions */
    template<typename xpu>
    class EdgeLayer : public ILayer {
    public:
        /*!\brief every virtual class must have virtual destructor */
        virtual ~EdgeLayer( void ){}
    protected:
        /*! \brief constructor */
        EdgeLayer(mshadow::Random<xpu> &rnd ) : rnd_(rnd) {}
        /*! \brief Gaussian initialization */
        template<int dim>
        inline void InitGaussian(mshadow::TensorContainer<xpu, dim> &mat, real_t sigma) {
            rnd_.SampleGaussian(mat, 0.0f, sigma);
        }
        /*! \brief Uniform initialization */
        template<int dim>
        inline void InitUniform(mshadow::TensorContainer<xpu, dim> &mat, real_t a, real_t b) {
            rnd_.SampleUniform(mat, a, b);
        }
        /*! \brief Xavier initialization */
        template<int dim>
        inline void InitXavier(mshadow::TensorContainer<xpu,dim> &mat, index_t in_num, index_t out_num) {
            real_t a = sqrt(3.0f / (in_num + out_num));
            this->InitUniform(mat, -a, a);
        }
        /*! \brief random number generator */
        mshadow::Random<xpu> &rnd_;
    }; // class EdgeLayer
}; // namespace cxxnet

namespace cxxnet {
    // simple fully connected layer that connects two nodes
    template<typename xpu>
    class FullConnectLayer : public EdgeLayer<xpu> {
    public:
        FullConnectLayer( mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out )
            : EdgeLayer<xpu>(rnd), in_(in), out_(out) {}
        virtual ~FullConnectLayer( void ){}
        virtual void Forward(bool is_train) {
            this->Forward( wmat_ );
        }
        virtual void Backprop(bool prop_grad){
            this->Backprop( prop_grad, wmat_ );
        }
        virtual void InitLayer( void ) {
            Assert( in_.is_mat(), "input need to be a matrix" );
            Assert( param_.num_hidden > 0, "must set nhidden correctly" );
            out_.data.shape = mshadow::Shape4( 1, 1, in_.data.shape[1], param_.num_hidden );
        }
        virtual void GetUpdaters( const char *updater, std::vector<IUpdater*> &updaters ){
            updaters.push_back( CreateUpdater( updater, EdgeLayer<xpu>::rnd_, wmat_, gwmat_, "wmat" ) );
            if( param_.no_bias == 0 ){
                updaters.push_back( CreateUpdater( updater, EdgeLayer<xpu>::rnd_, bias_, gbias_, "bias" ) );
            }
        }
        virtual void SetParam(const char *name, const char* val){
            param_.SetParam( name, val );
        }
        virtual void InitModel(void){
            // resize to correct shape
            wmat_.Resize( mshadow::Shape2( out_.data.shape[0], in_.data.shape[0] ) );
            bias_.Resize( mshadow::Shape1( out_.data.shape[0] ) );
            // random initalize
            if (param_.random_type == kGaussian) {
                EdgeLayer<xpu>::template InitGaussian<2> (wmat_, param_.init_sigma);
            } else {
                EdgeLayer<xpu>::template InitXavier<2> (wmat_, wmat_.shape[0], wmat_.shape[1]);
            }
            bias_ = param_.init_bias;
            // setup gradient weight
            gwmat_.Resize( wmat_.shape );
            gbias_.Resize( bias_.shape );
            gwmat_ = 0.0f; gbias_ = 0.0f;
        }
        virtual void SaveModel(mshadow::utils::IStream &fo) const{
            fo.Write( &param_, sizeof(LayerParam) );
            wmat_.SaveBinary( fo );
            bias_.SaveBinary( fo );
        }
        virtual void LoadModel(mshadow::utils::IStream &fi){
            Assert( fi.Read( &param_, sizeof(LayerParam) ) != 0, "load model");
            wmat_.LoadBinary( fi );
            bias_.LoadBinary( fi );
            // setup gradient weight
            gwmat_.Resize( wmat_.shape );
            gbias_.Resize( bias_.shape );
            gwmat_ = 0.0f; gbias_ = 0.0f;
        }
    protected:
        inline void Forward(mshadow::Tensor<xpu,2> wmat) {
            index_t nbatch = in_.data.shape[1];
            out_.mat()  = dot( in_.mat(), wmat.T() );
            if( param_.no_bias == 0 ){
                out_.mat() += repmat( bias_, nbatch );
            }

        }
        inline void Backprop(bool prop_grad, mshadow::Tensor<xpu,2> wmat){
            // accumulate gradient
            gwmat_ += dot( out_.mat().T(), in_.mat() );
            if( param_.no_bias == 0 ){
                gbias_ += sum_rows( out_.mat() );
            }
            // backprop
            if( prop_grad ){
                in_.mat() = dot( out_.mat(), wmat );
            }
        }
    protected:

        /*! \brief parameters that potentially be useful */
        LayerParam param_;
        /*! \brief input node */
        Node<xpu> &in_;
        /*! \brief output node */
        Node<xpu> &out_;
        /*! \brief weight matrix */
        // BDOL-NOTE: here is where the weight matrix is defined for a fully
        // connected layer
        mshadow::TensorContainer<xpu,2> wmat_;
        /*! \brief bias */
        mshadow::TensorContainer<xpu,1> bias_;
        /*! \brief accumulates the gradient of weight matrix */
        mshadow::TensorContainer<xpu,2> gwmat_;
        /*! \brief accumulates the gradient of bias */
        mshadow::TensorContainer<xpu,1> gbias_;
    };

    /*! \brief softmax layer, do softmax transformation during forward */
    template<typename xpu>
    class SoftmaxLayer: public ILayer{
    public:
        SoftmaxLayer( mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out )
            :out_(out){
            Assert( &in == &out, "softmax layer must self loop e.g layer[1->1] = softmax" );
        }
        virtual ~SoftmaxLayer( void ){}
        virtual void Forward(bool is_train){
            mshadow::Softmax( out_.mat(), out_.mat() );
        }
        virtual void Backprop(bool prop_grad){
        }
    private:
        /*! \brief only transform on out */
        Node<xpu> &out_;
    };

    template<typename xpu>
    class ConvolutionLayer : public EdgeLayer<xpu> {
    public:
        ConvolutionLayer( mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out )
            : EdgeLayer<xpu>(rnd), in_(in), out_(out) {
        }
        virtual ~ConvolutionLayer( void ){}
        virtual void Forward(bool is_train) {
            const index_t nbatch = in_.data.shape[3];
            for( index_t i = 0; i < nbatch; i += nstep_ ){
                // resize, incase last batch is smaller
                const index_t step = std::min(nstep_, nbatch-i);
                temp_col_.Resize( mshadow::Shape2( shape_colunit_[1], shape_colunit_[0]*step ) );
                temp_dst_.Resize( mshadow::Shape3( shape_dstunit_[2], shape_dstunit_[1], shape_dstunit_[0]*step ) );

                if( param_.pad == 0 ){
                    temp_col_ = unpack_patch2col( in_.data.Slice(i, i+step), param_.kernel_size, param_.stride );
                }else{
                    temp_col_ = unpack_patch2col( pad(in_.data.Slice(i,i+step),param_.pad), param_.kernel_size, param_.stride );
                }

                const index_t gstride = temp_col_.shape[1] / param_.num_group;
                for( int gid = 0; gid < param_.num_group; ++ gid ){
                    mshadow::Tensor<xpu,2> tmpc = temp_col_.Slice( gstride*gid, gstride*(gid+1) );
                    temp_dst_[ gid ] = dot( wmat_[gid], tmpc );
                }
                out_.data.Slice(i,i+step)  = swapaxis<2,3>( reshape( temp_dst_, mshadow::Shape4( param_.num_channel, step, out_.data.shape[1], out_.data.shape[0] ) ) );
            }
            if( param_.no_bias == 0 ){
                // add bias, broadcast bias to dim 2: channel
                out_.data += broadcast<2>( bias_, out_.data.shape );
            }
        }
        virtual void Backprop(bool prop_grad){
            const index_t nbatch = in_.data.shape[3];

            if( param_.no_bias == 0 ){
                gbias_ += sumall_except_dim<2>( out_.data );
            }

            for( index_t i = 0; i < nbatch; i += nstep_ ){
                const index_t step = std::min(nstep_, nbatch-i);
                temp_col_.Resize( mshadow::Shape2( shape_colunit_[1], shape_colunit_[0]*step ) );
                temp_dst_.Resize( mshadow::Shape3( shape_dstunit_[2], shape_dstunit_[1], shape_dstunit_[0]*step ) );

                temp_dst_ = reshape( swapaxis<2,3>( out_.data.Slice(i,i+step) ), temp_dst_.shape );

                if( param_.pad == 0 ){
                    temp_col_ = unpack_patch2col( in_.data.Slice(i, i+step), param_.kernel_size, param_.stride );
                }else{
                    temp_col_ = unpack_patch2col( pad(in_.data.Slice(i,i+step),param_.pad), param_.kernel_size, param_.stride );
                }

                const index_t gstride = temp_col_.shape[1] / param_.num_group;
                for( int gid = 0; gid < param_.num_group; ++ gid ){
                    mshadow::Tensor<xpu,2> tmpc = temp_col_.Slice( gstride*gid, gstride*(gid+1) );
                    gwmat_[gid] += dot( temp_dst_[gid], tmpc.T() );
                }

                if( prop_grad ){
                    for( int gid = 0; gid < param_.num_group; ++ gid ){
                        mshadow::Tensor<xpu,2> tmpc = temp_col_.Slice( gstride*gid, gstride*(gid+1) );
                        tmpc = dot( wmat_[gid].T(), temp_dst_[gid] );
                    }
                    if( param_.pad == 0 ){
                        in_.data.Slice(i,i+step) = pack_col2patch( temp_col_, in_.data.Slice(i,i+step).shape, param_.kernel_size, param_.stride );
                    }else{
                        mshadow::Shape<4> pshape = in_.data.Slice(i,i+step).shape; pshape[0] += 2*param_.pad; pshape[1] += 2*param_.pad;
                        in_.data.Slice(i,i+step) = crop( pack_col2patch( temp_col_, pshape, param_.kernel_size, param_.stride ), in_.data[i][0].shape );
                    }
                }
            }
        }
        virtual void InitLayer( void ) {
            const index_t ksize   = static_cast<index_t>( param_.kernel_size );
            const index_t kstride = static_cast<index_t>( param_.stride );
            Assert( in_.data.shape[2] % param_.num_group == 0,  "input channels must divide group size" );
            Assert( param_.num_channel % param_.num_group == 0, "output channels must divide group size" );
            Assert( param_.num_channel > 0, "must set nchannel correctly" );
            Assert( param_.kernel_size > 0, "must set kernel_size correctly" );
            Assert( ksize <= in_.data.shape[0] && ksize <= in_.data.shape[1], "kernel size exceed input" );

            mshadow::Shape<4> oshape = mshadow::
                Shape4( in_.data.shape[3], param_.num_channel,
                        (in_.data.shape[1] + 2 * param_.pad - ksize)/kstride + 1,
                        (in_.data.shape[0] + 2 * param_.pad - ksize)/kstride + 1 );
            out_.data.shape = oshape;
            // this is the unit size of eacj temp structure
            shape_colunit_ = mshadow::Shape2( in_.data.shape[2]*ksize*ksize, oshape[1]*oshape[0] );
            shape_dstunit_ = mshadow::Shape3( param_.num_group, param_.num_channel/param_.num_group, oshape[1]*oshape[0] );
            nstep_ = std::max( std::min( (index_t)(param_.temp_col_max / shape_colunit_.Size()), in_.data.shape[3] ), 1U );
            // make nstep more balanced,  nstep will use exactly same number of operations to finish,
            index_t nop = (in_.data.shape[3]+nstep_-1) / nstep_;
            nstep_ = (in_.data.shape[3] + nop - 1 )/ nop;
            utils::Assert( nstep_ > 0 );

            // helper structure
            temp_col_.Resize( mshadow::Shape2( shape_colunit_[1], shape_colunit_[0]*nstep_ ));
            temp_dst_.Resize( mshadow::Shape3( shape_dstunit_[2], shape_dstunit_[1], shape_dstunit_[0]*nstep_) );

            if( param_.silent == 0 ){
                printf("ConvolutionLayer: nstep=%u\n", nstep_ );
            }
        }
        virtual void GetUpdaters( const char *updater, std::vector<IUpdater*> &updaters ){
            updaters.push_back( CreateUpdater( updater, EdgeLayer<xpu>::rnd_, wmat_, gwmat_, "wmat" ) );
            if( param_.no_bias == 0 ){
                updaters.push_back( CreateUpdater( updater, EdgeLayer<xpu>::rnd_, bias_, gbias_, "bias" ) );
            }
        }
        virtual void SetParam(const char *name, const char* val){
            param_.SetParam( name, val );
        }
        virtual void InitModel(void){
            // resize to correct shape, use 2d to store the weight, since we use dot
            wmat_.Resize( mshadow::Shape3( param_.num_group, param_.num_channel / param_.num_group,
                                           in_.data.shape[2] / param_.num_group * param_.kernel_size * param_.kernel_size ) );
            bias_.Resize( mshadow::Shape1( param_.num_channel ) );

            if (param_.random_type == kGaussian) {
                EdgeLayer<xpu>::InitGaussian(wmat_, param_.init_sigma);
            } else {
                EdgeLayer<xpu>::InitXavier(wmat_, wmat_.shape[1], wmat_.shape[0]);
            }
            bias_ = param_.init_bias;
            // setup gradient
            gwmat_.Resize( wmat_.shape );
            gbias_.Resize( bias_.shape );
            gwmat_ = 0.0f; gbias_ = 0.0f;
        }
        virtual void SaveModel(mshadow::utils::IStream &fo) const{
            fo.Write( &param_, sizeof(LayerParam) );
            wmat_.SaveBinary( fo );
            bias_.SaveBinary( fo );
        }
        virtual void LoadModel(mshadow::utils::IStream &fi){
            Assert( fi.Read( &param_, sizeof(LayerParam) ) != 0, "load model");
            wmat_.LoadBinary( fi );
            bias_.LoadBinary( fi );
            // setup gradient
            gwmat_.Resize( wmat_.shape );
            gbias_.Resize( bias_.shape );
            gwmat_ = 0.0f; gbias_ = 0.0f;
        }
    private:
        /*! \brief parameters that potentially be useful */
        LayerParam param_;
        /*! \brief input node */
        Node<xpu> &in_;
        /*! \brief output node */
        Node<xpu> &out_;
        /*! \brief weight matrix */
        mshadow::TensorContainer<xpu,3> wmat_;
        /*! \brief bias */
        mshadow::TensorContainer<xpu,1> bias_;
        /*! \brief accumulates the gradient of weight matrix */
        mshadow::TensorContainer<xpu,3> gwmat_;
        /*! \brief accumulates the gradient of bias */
        mshadow::TensorContainer<xpu,1> gbias_;
        /*! \brief temporary data structure to store patches */
        mshadow::TensorContainer<xpu,2> temp_col_;
        /*! \brief temporary data structure to store results */
        mshadow::TensorContainer<xpu,3> temp_dst_;
        /*! \brief shape of column unit */
        mshadow::Shape<2> shape_colunit_;
        /*! \brief shape of dst unit */
        mshadow::Shape<3> shape_dstunit_;
        /*! \brief how many number of batches to be unpacked together */
        mshadow::index_t nstep_;
    };

    template<typename Reducer, bool scalebysize, typename xpu>
    class PoolingLayer : public ILayer {
    public:
        PoolingLayer(Node<xpu> &in, Node<xpu> &out)
            : in_(in), out_(out) {
        }
        virtual ~PoolingLayer() {}
        virtual void Forward(bool is_train) {
            const int ksize = param_.kernel_size;
            mshadow::Shape<2> pshape = out_.data[0][0].shape;
            if( !scalebysize ){
                tmp_ = pool<Reducer>(in_.data, pshape, ksize, param_.stride);
            }else{
                tmp_ = pool<Reducer>(in_.data, pshape, ksize, param_.stride) * (1.0f/(ksize*ksize) );
            }
            mshadow::Copy( out_.data, tmp_ );
        }
        virtual void Backprop(bool prop_grad) {
            if (prop_grad) {
                const int ksize = param_.kernel_size;
                if( !scalebysize ){
                    in_.data = unpool<Reducer>(in_.data, tmp_, out_.data, param_.kernel_size, param_.stride);
                }else{
                    in_.data = unpool<Reducer>(in_.data, tmp_, out_.data, param_.kernel_size, param_.stride) * (1.0f/(ksize*ksize) );
                }
            }
        }
        virtual void SetParam(const char *name, const char* val) {
            param_.SetParam( name, val );
        }
        virtual void InitLayer() {
            const index_t ksize   = static_cast<index_t>( param_.kernel_size );
            const index_t kstride = static_cast<index_t>( param_.stride );
            Assert( param_.kernel_size > 0, "must set kernel_size correctly" );
            Assert( ksize <= in_.data.shape[0] && ksize <= in_.data.shape[1], "kernel size exceed input" );
            // conform to same shape style as caffe, though maybe not necessary
            mshadow::Shape<4> oshape = mshadow::
                Shape4( in_.data.shape[3], in_.data.shape[2],
                        (in_.data.shape[1] - ksize + kstride-1)/kstride + 1,
                        (in_.data.shape[0] - ksize + kstride-1)/kstride + 1 );
            tmp_.Resize( oshape ); out_.data.shape = oshape;
        }
    private:
        /*! \brief parameters that potentially be useful */
        LayerParam param_;
        /*! \brief input node */
        Node<xpu> &in_;
        /*! \brief output node */
        Node<xpu> &out_;
        /*! \brief pooled result */
        mshadow::TensorContainer<xpu, 4> tmp_;
    }; // class PoolingLayer


    template<typename xpu,typename ForwardOp, typename BackOp >
    class ActivationLayer : public ILayer{
    public:
        ActivationLayer( Node<xpu> &in, Node<xpu> &out )
            :in_(in), out_(out) {
        }
        virtual ~ActivationLayer( void ){}
        virtual void Forward( bool is_train ) {
            in_.data = F<ForwardOp>( in_.data );
            mshadow::Copy( out_.data, in_.data );
        }
        virtual void Backprop( bool prop_grad ){
            in_.data = F<BackOp>( in_.data ) * out_.data;
        }
        virtual void InitLayer( void ) {
            out_.data.shape = in_.data.shape;
        }
    private:
        /*! \brief input node */
        Node<xpu> &in_;
        /*! \brief output node */
        Node<xpu> &out_;
    };
};

namespace cxxnet {
    template<typename xpu>
    class PaddingLayer : public ILayer {
    public:
        PaddingLayer(Node<xpu> &in, Node<xpu> &out)
            : in_(in), out_(out) {
        }
        virtual ~PaddingLayer(){}
        virtual void SetParam(const char *name, const char *val) {
            if (!strcmp(name, "pad")) pad_ = static_cast<index_t>(atoi(val));
        }
        virtual void Forward(bool is_train) {
            out_.data = pad(in_.data, pad_);
        }
        virtual void Backprop(bool prop_grad) {
            if (prop_grad) {
                in_.data = crop(out_.data, in_.data[0][0].shape);
            }
        }
        virtual void InitLayer() {
            mshadow::Shape<4> oshape = mshadow::Shape4(in_.data.shape[3], in_.data.shape[2],
                                              in_.data.shape[1] + 2 * pad_,
                                              in_.data.shape[0] + 2 * pad_);
            out_.data.shape = oshape;
        }
    private:
        /*! \brief input node */
        Node<xpu> &in_;
        /*! \brief output node */
        Node<xpu> &out_;
        /*! \brief padding size */
        index_t pad_;
    }; // class padding layer
}; // namespace cxxnet

namespace cxxnet{
    template<typename xpu>
    class LRNLayer : public ILayer {
    public:
        LRNLayer(Node<xpu> &in, Node<xpu> &out)
            : in_(in), out_(out) {
            // default values
            this->knorm_ = 1.0f;
            this->nsize_ = 3;
        }
        virtual ~LRNLayer( void ){}
        virtual void SetParam(const char *name, const char *val) {
            if (!strcmp(name, "local_size")) nsize_ = static_cast<index_t>( atoi(val) );
            if (!strcmp(name, "alpha"))      alpha_ = static_cast<real_t>( atof(val) );
            if (!strcmp(name, "beta"))       beta_  = static_cast<real_t>( atof(val) );
            if (!strcmp(name, "knorm"))      knorm_ = static_cast<real_t>( atof(val) );
        }
        virtual void Forward(bool is_train) {
            using namespace mshadow;
            const real_t salpha = alpha_ / nsize_;
            // stores normalizer without power
            tmp_norm = chpool<red::sum>( F<op::square>( in_.data ) , nsize_ ) * salpha + knorm_;
            out_.data = in_.data * F<op::power>( tmp_norm, -beta_ );
        }
        virtual void Backprop(bool prop_grad) {
            using namespace mshadow;
            const real_t salpha = alpha_ / nsize_;
            if( prop_grad ) {
                // backup input data
                mshadow::Copy( tmp_in, in_.data );
                // first gradient to a[i], will be 1 / normalizer
                in_.data = out_.data * F<op::power>( tmp_norm, -beta_ );
                // gradient to normalizer
                in_.data += ( - 2.0f * beta_ * salpha ) * chpool<red::sum>( out_.data * tmp_in * F<op::power>( tmp_norm, -beta_-1.0f ), nsize_ )  * tmp_in;
            }
        }
        virtual void InitLayer( void ) {
            out_.data.shape = in_.data.shape;
            tmp_in.Resize( in_.data.shape );
            tmp_norm.Resize( in_.data.shape );
        }
    private:
        /*! \brief input node */
        Node<xpu> &in_;
        /*! \brief output node */
        Node<xpu> &out_;
        /*! \brief input temp data */
        mshadow::TensorContainer<xpu,4> tmp_in;
        /*! \brief temp normalizer */
        mshadow::TensorContainer<xpu,4> tmp_norm;
        /*! \brief alpha */
        real_t alpha_;
        /*! \brief beta */
        real_t beta_;
        /*! \brief knorm */
        real_t knorm_;
        /*! \brief neighbor size*/
        index_t nsize_;
    }; // class lrn layer
}; // namespace cxxnet

namespace cxxnet{
    template<typename xpu>
    class FlattenLayer : public ILayer{
    public:
        FlattenLayer( Node<xpu> &in, Node<xpu> &out )
            :in_(in), out_(out) {
        }
        virtual ~FlattenLayer( void ){}
        virtual void Forward( bool is_train ) {
            out_.data = reshape( in_.data, out_.data.shape );
        }
        virtual void Backprop( bool prop_grad ){
            if( prop_grad ){
                in_.data = reshape( out_.data, in_.data.shape );
            }
        }
        virtual void InitLayer( void ) {
            mshadow::Shape<4> ishape = in_.data.shape;
            out_.data.shape = mshadow::Shape4( 1, 1, ishape[3], ishape[2]*ishape[1]*ishape[0] );
        }
    private:
        /*! \brief input node */
        Node<xpu> &in_;
        /*! \brief output node */
        Node<xpu> &out_;
    };
};

namespace cxxnet {
    template<typename xpu>
    class DropConnLayer : public FullConnectLayer<xpu> {
    public:
        DropConnLayer(mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out)
            : Parent(rnd, in, out) {}
        virtual void Forward(bool is_train) {
            if( is_train ){
                const real_t pkeep = 1.0f - Parent::param_.dropout_threshold;
                mask_ = F<op::threshold>( Parent::rnd_.uniform( mask_.shape ), pkeep ) * (1.0f/pkeep);
                tmpw_ = this->wmat_ * mask_;
            }else{
                mshadow::Copy( tmpw_, this->wmat_ );
            }
            Parent::Forward( tmpw_ );
        }
        virtual void Backprop(bool prop_grad) {
            Parent::Backprop( prop_grad, tmpw_ );
            Parent::gwmat_ *= mask_;
        }
        virtual void InitLayer( void ){
            Parent::InitLayer();
            this->mask_.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
            this->tmpw_.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
        }
    private:
        typedef FullConnectLayer<xpu> Parent;
    private:
        mshadow::TensorContainer<xpu, 2> mask_, tmpw_;
    }; // class DropconnLayer

    /**
     * DPP Dropout. Brian Dolhansky 06/2014
     *  Because we need information about the weights, we overload the
     *  FullConnectLayer class instead of the DropoutLayer class. To use DPP
     *  dropout, replace a connection between two layers with:
     *    layer[i->j] = dpp_dropout:label
     *      threshold = p
     *      nhidden = n
     *      init_sigma = 0.01 (example)
     *
     *  This class samples k=p*n points from a k-DPP, and then drops those units
     *  out. Thus, we are sampling a diverse set and removing it for the current
     *  epoch. Currently I'm using a hard-coded kernel L = (W*W^T).^2. In the
     *  future I will add the ability to change kernels programatically in the
     *  configuration file.
     */
    template<typename xpu>
    class DppDropoutLayer : public FullConnectLayer<xpu> {
    public:
        DppDropoutLayer(mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out)
            : Parent(rnd, in, out) {}
        virtual void Forward(bool is_train) {
            // If we're training, we need to do dropout
            if( is_train ){
                
                // Form the L-ensemble L = (W*W^T).^2
                L_ = dot(this->wmat_.T(), this->wmat_);
                Lsq_ =  L_ * L_;

                // Copy the mshadow matrix into my custom Matrix container.
                // TODO: I should probably change the DPP class to accept
                // mshadow data containers.
                Matrix<real_t>* Lm = new Matrix<real_t>(Lsq_.dptr, Lsq_.shape[1], Lsq_.shape[0], false);
                DPP<real_t>* dpp = new DPP<real_t>(Lm);

                // Calculate the k for a k-DPP
                real_t pkeep = 1.0f - Parent::param_.dropout_threshold;
                int k = (int)(pkeep*Lm->h());

                // Sample the DPP and store the samples
                Matrix<real_t>* Y = dpp->sample(k);

                // For all DPP samples, we set drop_idx to true
                std::vector<bool> drop_idx(Lm->h(), false);
                for (int i=0; i<k; i++) {
                  drop_idx[(int)Y->get(i, 0)] = true;
                }

                // For a unit that we've dropped, set ALL weights in the
                // corresponding row (weights going to the next layer) to 0
                for (unsigned int j=0; j<mask_.shape[0]; j++) {
                  if (drop_idx[j]) {
                    for (unsigned int i=0; i<mask_.shape[1]; i++) {
                      mask_[i][j] = 0.0;
                    }
                  // Otherwise, we scale the weight up to account for the fact
                  // that we're only keeping pkeep*n units. We could just as
                  // easily scale during testing.
                  } else {
                    for (unsigned int i=0; i<mask_.shape[1]; i++) {
                      mask_[i][j] = 1.0/pkeep;
                    }
                  }
                }

                // Multiply the temporary weight matrix (used in forward
                // propagation) by the scaled dropout mask
                tmpw_ = this->wmat_*mask_;

                delete Lm;
                delete dpp;
                delete Y;
            }else{
                mshadow::Copy( tmpw_, this->wmat_ );
            }
            Parent::Forward( tmpw_ );
        }

        // For backpropagation, make sure we also do dropout
        virtual void Backprop(bool prop_grad) {
            Parent::Backprop( prop_grad, tmpw_ );
            Parent::gwmat_ *= mask_;
        }

        virtual void InitLayer( void ){
            Parent::InitLayer();
            this->mask_.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
            this->tmpw_.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
            // This matrix will hold the DPP L-ensemble
            this->L_.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
            this->Lsq_.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
        }
    private:
        typedef FullConnectLayer<xpu> Parent;
    private:
        mshadow::TensorContainer<xpu, 2> mask_, tmpw_, L_, Lsq_;
    }; // class DppDropoutLayer

    template<typename xpu>
    class DppDropoutOnlySimilarityLayer : public FullConnectLayer<xpu> {
    public:
        DppDropoutOnlySimilarityLayer(mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out)
            : Parent(rnd, in, out) {}
        virtual void Forward(bool is_train) {
            // If we're training, we need to do dropout
            if( is_train ){
                
                w_normed = this->wmat_+0.0;
                for (unsigned int j=0; j<this->wmat_.shape[0]; j++) {
                  real_t w_normalizer = 0.0;
                  for (unsigned int i=0; i<this->wmat_.shape[1]; i++) {
                    w_normalizer += this->wmat_[i][j]*this->wmat_[i][j];
                  }

                  w_normalizer = sqrt(w_normalizer);
                  
                  for (unsigned int i=0; i<this->wmat_.shape[1]; i++) {
                    w_normed[i][j] = w_normed[i][j]/w_normalizer;
                  }
                }

                // Form the L-ensemble L = (W*W^T).^2
                L_ = dot(w_normed.T(), w_normed);
                Lsq_ =  L_ * L_;

                // Copy the mshadow matrix into my custom Matrix container.
                // TODO: I should probably change the DPP class to accept
                // mshadow data containers.
                Matrix<real_t>* Lm = new Matrix<real_t>(Lsq_.dptr, Lsq_.shape[1], Lsq_.shape[0], false);
                DPP<real_t>* dpp = new DPP<real_t>(Lm);

                // Calculate the k for a k-DPP
                real_t pkeep = 1.0f - Parent::param_.dropout_threshold;
                int k = (int)(pkeep*Lm->h());

                // Sample the DPP and store the samples
                Matrix<real_t>* Y = dpp->sample(k);

                // For all DPP samples, we set drop_idx to true
                std::vector<bool> drop_idx(Lm->h(), false);
                for (int i=0; i<k; i++) {
                  drop_idx[(int)Y->get(i, 0)] = true;
                }

                // For a unit that we've dropped, set ALL weights in the
                // corresponding row (weights going to the next layer) to 0
                for (unsigned int j=0; j<mask_.shape[0]; j++) {
                  if (drop_idx[j]) {
                    for (unsigned int i=0; i<mask_.shape[1]; i++) {
                      mask_[i][j] = 0.0;
                    }
                  // Otherwise, we scale the weight up to account for the fact
                  // that we're only keeping pkeep*n units. We could just as
                  // easily scale during testing.
                  } else {
                    for (unsigned int i=0; i<mask_.shape[1]; i++) {
                      mask_[i][j] = 1.0/pkeep;
                    }
                  }
                }

                // Multiply the temporary weight matrix (used in forward
                // propagation) by the scaled dropout mask
                tmpw_ = this->wmat_*mask_;

                delete Lm;
                delete dpp;
                delete Y;
            }else{
                mshadow::Copy( tmpw_, this->wmat_ );
            }
            Parent::Forward( tmpw_ );
        }

        // For backpropagation, make sure we also do dropout
        virtual void Backprop(bool prop_grad) {
            Parent::Backprop( prop_grad, tmpw_ );
            Parent::gwmat_ *= mask_;
        }

        virtual void InitLayer( void ){
            Parent::InitLayer();
            this->mask_.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
            this->tmpw_.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
            // This matrix will hold the DPP L-ensemble
            this->L_.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
            this->Lsq_.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
            this->w_normed.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
        }
    private:
        typedef FullConnectLayer<xpu> Parent;
    private:
        mshadow::TensorContainer<xpu, 2> mask_, tmpw_, L_, Lsq_;
        mshadow::TensorContainer<xpu, 2> w_normed;
    }; // class DppDropoutOnlySimilarityLayer

    template<typename xpu>
    class DppDropoutOnlyQualityLayer : public FullConnectLayer<xpu> {
    public:
        DppDropoutOnlyQualityLayer(mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out)
            : Parent(rnd, in, out) {}

        virtual void Forward(bool is_train) {
            if( is_train ){
                const real_t pkeep = 1.0f - Parent::param_.dropout_threshold;

                mask_ = F<op::threshold>( Parent::rnd_.uniform( mask_.shape ), pkeep ) * (1.0f/pkeep);
                int N = mask_.shape[0];

                // Sample k from a binomial distribution
                // Not fancy but works for now
                int k=0;
                for (int i=0; i<N; i++) {
                  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                  if (r > pkeep) {
                    k++;
                  }
                }

                int count = 0;
                std::vector<bool> keep_idx(N, false);
                // Select an item with probability
                //  (w^Tw)^2 / [ (w^Tw)^2 + 1 ]
                for (unsigned int j=0; j<this->wmat_.shape[0]; j++) {
                  real_t w_norm = 0.0;
                  for (unsigned int i=0; i<this->wmat_.shape[1]; i++) {
                    w_norm += this->wmat_[i][j]*this->wmat_[i][j];
                  }
                  real_t p = w_norm*w_norm / (w_norm*w_norm + 1);
                  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                  if (r > p) {
                    keep_idx[j] = true;
                    count++;
                  }
                }

                if (count < k) {
                  for (int i=count; i<k; i++) {
                    int j = rand() % N;
                    while (keep_idx[j]) {
                      j = rand() % N;
                    }
                    keep_idx[j] = true;
                  }
                }

                for (unsigned int i=0; i<mask_.shape[0]; i++) {
                  for (unsigned int j=0; j<mask_.shape[1]; j++) {
                    if (keep_idx[i]) {
                      mask_[0][0][j][i]  = 1.0f/pkeep;
                    } else {
                      mask_[0][0][j][i]  = 0.0;
                    }
                  }
                }

                this->in_.data = this->in_.data * mask_;
            }

            Parent::Forward( this->wmat_ );
        }
        virtual void Backprop(bool prop_grad) {
            Parent::Backprop( prop_grad, this->wmat_ );
            if (prop_grad) {
                this->in_.data *= mask_;
            }
        }
        virtual void InitLayer( void ){
            Parent::InitLayer();
            mask_.Resize( this->in_.data.shape );
        }
    private:
        typedef FullConnectLayer<xpu> Parent;
    private:
        mshadow::TensorContainer<xpu, 4> mask_;
    }; // class DppDropoutOnlyQualityLayer

    /**
     * DPP Dropin. Brian Dolhansky 06/2014
     *  Because we need information about the weights, we overload the
     *  FullConnectLayer class instead of the DropoutLayer class. To use DPP
     *  dropout, replace a connection between two layers with:
     *    layer[i->j] = dpp_dropin:label
     *      threshold = p
     *      nhidden = n
     *      init_sigma = 0.01 (example)
     *
     *  This class samples k=p*n points from a k-DPP, and then drops the OTHER
     *  units . Thus, we are sampling a diverse set and removing all others for
     *  the current epoch. Currently I'm using a hard-coded kernel L =
     *  (W*W^T).^2.  In the future I will add the ability to change kernels
     *  programatically in the configuration file.
     */
    template<typename xpu>
    class DppDropinLayer : public FullConnectLayer<xpu> {
    public:
        DppDropinLayer(mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out)
            : Parent(rnd, in, out) {}
        virtual void Forward(bool is_train) {
            if( is_train ){
                L_ = dot(this->wmat_.T(), this->wmat_);
                Lsq_ =  L_ * L_;

                Matrix<real_t>* Lm = new Matrix<real_t>(Lsq_.dptr, Lsq_.shape[1], Lsq_.shape[0], false);
                DPP<real_t>* dpp = new DPP<real_t>(Lm);
                real_t pkeep = 1.0f - Parent::param_.dropout_threshold;
                int k = (int)(pkeep*Lm->h());
                Matrix<real_t>* Y = dpp->sample(k);

                // This is the only change from DPP Dropout: for all DPP
                // samples, set drop_idx to FALSE
                std::vector<bool> drop_idx(Lm->h(), true);
                for (int i=0; i<k; i++) {
                  drop_idx[(int)Y->get(i, 0)] = false;
                }

                for (unsigned int j=0; j<mask_.shape[0]; j++) {
                  if (drop_idx[j]) {
                    for (unsigned int i=0; i<mask_.shape[1]; i++) {
                      mask_[i][j] = 0.0f;
                    }
                  } else {
                    for (unsigned int i=0; i<mask_.shape[1]; i++) {
                      mask_[i][j] = 1.0f/pkeep;
                    }
                  }
                }

                tmpw_ = this->wmat_*mask_;

                delete Lm;
                delete dpp;
                delete Y;
            }else{
                mshadow::Copy( tmpw_, this->wmat_ );
            }
            Parent::Forward( tmpw_ );
        }
        virtual void Backprop(bool prop_grad) {
            Parent::Backprop( prop_grad, tmpw_ );
            Parent::gwmat_ *= mask_;
        }
        virtual void InitLayer( void ){
            Parent::InitLayer();
            this->mask_.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
            this->tmpw_.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
            // This matrix will hold the DPP L-ensemble
            this->L_.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
            this->Lsq_.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
        }
    private:
        typedef FullConnectLayer<xpu> Parent;
    private:
        mshadow::TensorContainer<xpu, 2> mask_, tmpw_, L_, Lsq_;
    }; // class DppDropinLayer

    template<typename xpu>
    class DppDropinOnlySimilarityLayer : public FullConnectLayer<xpu> {
    public:
        DppDropinOnlySimilarityLayer(mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out)
            : Parent(rnd, in, out) {}
        virtual void Forward(bool is_train) {
            if( is_train ){
                w_normed = this->wmat_+0.0;
                for (unsigned int j=0; j<this->wmat_.shape[0]; j++) {
                  real_t w_normalizer = 0.0;
                  for (unsigned int i=0; i<this->wmat_.shape[1]; i++) {
                    w_normalizer += this->wmat_[i][j]*this->wmat_[i][j];
                  }

                  w_normalizer = sqrt(w_normalizer);
                  
                  for (unsigned int i=0; i<this->wmat_.shape[1]; i++) {
                    w_normed[i][j] = w_normed[i][j]/w_normalizer;
                  }
                }

                // Form the L-ensemble L = (W*W^T).^2
                L_ = dot(w_normed.T(), w_normed);
                Lsq_ =  L_ * L_;

                Matrix<real_t>* Lm = new Matrix<real_t>(Lsq_.dptr, Lsq_.shape[1], Lsq_.shape[0], false);
                DPP<real_t>* dpp = new DPP<real_t>(Lm);
                real_t pkeep = 1.0f - Parent::param_.dropout_threshold;
                int k = (int)(pkeep*Lm->h());
                Matrix<real_t>* Y = dpp->sample(k);

                // This is the only change from DPP Dropout: for all DPP
                // samples, set drop_idx to FALSE
                std::vector<bool> drop_idx(Lm->h(), true);
                for (int i=0; i<k; i++) {
                  drop_idx[(int)Y->get(i, 0)] = false;
                }

                for (unsigned int j=0; j<mask_.shape[0]; j++) {
                  if (drop_idx[j]) {
                    for (unsigned int i=0; i<mask_.shape[1]; i++) {
                      mask_[i][j] = 0.0f;
                    }
                  } else {
                    for (unsigned int i=0; i<mask_.shape[1]; i++) {
                      mask_[i][j] = 1.0f/pkeep;
                    }
                  }
                }

                tmpw_ = this->wmat_*mask_;

                delete Lm;
                delete dpp;
                delete Y;
            }else{
                mshadow::Copy( tmpw_, this->wmat_ );
            }
            Parent::Forward( tmpw_ );
        }
        virtual void Backprop(bool prop_grad) {
            Parent::Backprop( prop_grad, tmpw_ );
            Parent::gwmat_ *= mask_;
        }
        virtual void InitLayer( void ){
            Parent::InitLayer();
            this->mask_.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
            this->tmpw_.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
            // This matrix will hold the DPP L-ensemble
            this->L_.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
            this->Lsq_.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
            this->w_normed.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
        }
    private:
        typedef FullConnectLayer<xpu> Parent;
    private:
        mshadow::TensorContainer<xpu, 2> mask_, tmpw_, L_, Lsq_;
        mshadow::TensorContainer<xpu, 2> w_normed;
    }; // class DppDropinOnlySimilarityLayer

    /**
     * Stratified Sampling. Brian Dolhansky 2014.
     * Use stratified sampling to select units to dropin.
     */
    template<typename xpu>
    class StratifiedSamplingLayer : public FullConnectLayer<xpu> {
    public:
        StratifiedSamplingLayer(mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out)
            : Parent(rnd, in, out) {}
        virtual void Forward(bool is_train) {
            if( is_train ){
              // Number of rows = dimensionality of input
              // Number cols = dimensionality of output
              // n = # ROWS
              int n = this->wmat_.shape[1];
              // m = # COLS
              int m = this->wmat_.shape[0];

              // DEBUG: write the weight matrix
              //std::ofstream f;
              //f.open("wtest");
              //for (int i=0; i<m; i++) {
                //for (int j=0; j<n; j++) {
                  //f << this->wmat_[i][j] << " ";
                //}
                //f << std::endl;
              //}
              //f.close();


              // Each column of the weight matrix is a data point
              // There are m columns (m objects) and n rows (n features)
              float** objects = (float**)malloc(m * sizeof(float*));
              objects[0] = (float*) malloc(n*m * sizeof(float));
              for (int i=1; i<m; i++) {
                objects[i] = objects[i-1] + 2;
              }

              // TODO: check to make sure this is right
              for (int i=0; i<m; i++) {
                for (int j=0; j<n; j++) {
                  objects[i][j] = this->wmat_[i][j];
                }
              }


              float** clusters;
              float threshold = 1E-8;
              int loopIterations;
              int* membership = (int*) malloc(m * sizeof(int));
              int k = 10;
              clusters = seq_kmeans(objects, n, m, k, threshold,
                                      membership, &loopIterations);

              free(objects[0]);
              free(objects);

              // TODO: parameterize this
              int numPerCluster = 5;

              std::vector<int> kmeansSamples;
              for (int i=0; i<k; i++) {
                std::vector<int> clusterIdx;
                int nCluster = 0;
                for (int j=0; j<m; j++) {
                  if (membership[j] == i) {
                    clusterIdx.push_back(j);
                    nCluster++;
                  }
                }

                if (clusterIdx.size() <= numPerCluster) {
                  for (unsigned int j=0; j<clusterIdx.size(); j++) {
                    kmeansSamples.push_back( clusterIdx[j] );
                  }
                } else {
                  std::vector<int> sampleIdx = this->sampleWithoutReplacement(nCluster, numPerCluster);
                  for (int j=0; j<numPerCluster; j++) {
                    kmeansSamples.push_back( clusterIdx[sampleIdx[j]] );

                  }
                }

              }
              free(membership);
              free(clusters);

              std::vector<bool> drop_idx(m, true);
              //for (int i=0; i<m; i++) {
                //double r = (double) rand() / (RAND_MAX);
                //if (r <= 0.5) {
                  //drop_idx[i] = false;
                //}
              //}
                
              for (unsigned int i=0; i<kmeansSamples.size(); i++) {
                drop_idx[kmeansSamples[i]] = false;
                whist[kmeansSamples[i]] = whist[kmeansSamples[i]]+1;
              }

              if (this->wmat_.shape[0] == 102) {
                std::ofstream f102;
                f102.open("kmeans102");
                for (int i=0; i<102; i++) {
                  f102 << whist[i] << std::endl;
                }
                f102.close();
              } else if (this->wmat_.shape[0] == 101) {
                std::ofstream f101;
                f101.open("kmeans101");
                for (int i=0; i<101; i++) {
                  f101 << whist[i] << std::endl;
                }
                f101.close();
              } else if (this->wmat_.shape[0] == 100) {
                std::ofstream f100;
                f100.open("kmeans100");
                for (int i=0; i<100; i++) {
                  f100 << whist[i] << std::endl;
                }
                f100.close();
              }

              real_t pkeep = 1.0f - Parent::param_.dropout_threshold;
              for (unsigned int j=0; j<mask_.shape[0]; j++) {
                if (drop_idx[j]) {
                  for (unsigned int i=0; i<mask_.shape[1]; i++) {
                    mask_[i][j] = 0.0f;
                  }
                } else {
                  for (unsigned int i=0; i<mask_.shape[1]; i++) {
                    mask_[i][j] = 1.0f/pkeep;
                  }
                }
              }

              tmpw_ = this->wmat_*mask_;
            }else{
                mshadow::Copy( tmpw_, this->wmat_ );
            }
            Parent::Forward( tmpw_ );
        }
        virtual void Backprop(bool prop_grad) {
            Parent::Backprop( prop_grad, tmpw_ );
            Parent::gwmat_ *= mask_;
        }
        virtual void InitLayer( void ){
            Parent::InitLayer();
            this->mask_.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
            this->tmpw_.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );

            for (unsigned int i=0; i<this->in_.data.shape[0]; i++) {
              whist.push_back(0);
            }

        }
    private:
        typedef FullConnectLayer<xpu> Parent;

        std::vector<int> sampleWithoutReplacement(int n, int num_samples) {
          std::vector<int> a(n, 0);
          for (int i=0; i<n; i++) {
            a[i] = i;
          }

          int mx = n-1;
          while (mx > 0) {
            int r = (int)( (double)mx*((double) rand() / (RAND_MAX)) );
            int tmp = a[mx];
            a[mx] = a[r];
            a[r] = tmp;
            mx--;
          }

          return a;
        }
        
    private:
        mshadow::TensorContainer<xpu, 2> mask_, tmpw_, L_, Lsq_;
        std::vector<int> whist;
    }; // class StratifiedSamplingLayer

    template<typename xpu>
    class SimBasedDropinLayer : public FullConnectLayer<xpu> {
    public:
        SimBasedDropinLayer(mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out)
            : Parent(rnd, in, out) {}

        struct elemwise_log {
          MSHADOW_XINLINE static real_t Map(real_t a) {
            if (a < 1E-8) {
              return -18.0; // Approximately log(1E-8), a form of Lapacian smoothing
            } else {
              return log(a);
            }
          }
        };
        struct elemwise_sqrt {
          MSHADOW_XINLINE static real_t Map(real_t a) {
            return sqrt(a);
          }
        };
        struct elemwise_square {
          MSHADOW_XINLINE static real_t Map(real_t a) {
            return a*a;
          }
        };

        real_t logsumexp(std::vector<real_t> v) {
          // Find max
          real_t mx = v[0];
          for (int i=1; i<v.size(); i++) {
            if (v[i] > mx) {
              mx = v[i];
            }
          }

          real_t s = 0.0;
          // Log-sum-exp trick
          for (int i=0; i<v.size(); i++) {
            s += exp(v[i] - mx);
          }

          real_t lse = mx + log(s);

          return lse;
        }

        virtual void Forward(bool is_train) {
            if( is_train ){
                const real_t pkeep = 1.0f - Parent::param_.dropout_threshold;

                // The mask is a 4D tensor with the dimensions [ D , n_batch , 1, 1]
                // You CAN directly set mask entries if it's on the GPU. I don't
                // get it..
                mask_ = F<op::threshold>( Parent::rnd_.uniform( mask_.shape ), pkeep ) * (1.0f/pkeep);
                int N = mask_.shape[0];

                // BEGIN SIM-BASED SAMPLING

                // First we normalize all the vectors
                w_normed = this->wmat_+0.0;
                for (unsigned int j=0; j<this->wmat_.shape[0]; j++) {
                  real_t w_normalizer = 0.0;
                  for (unsigned int i=0; i<this->wmat_.shape[1]; i++) {
                    w_normalizer += this->wmat_[i][j]*this->wmat_[i][j];
                  }

                  w_normalizer = sqrt(w_normalizer);
                  
                  for (unsigned int i=0; i<this->wmat_.shape[1]; i++) {
                    w_normed[i][j] = w_normed[i][j]/w_normalizer;
                  }
                }

                L_ = dot(w_normed.T(), w_normed);
                D_ =  (1 - L_ * L_);
                D_ = F<elemwise_log>(D_);

                // Sample k from a binomial distribution
                // Not fancy but works for now
                int k=0;
                for (int i=0; i<N; i++) {
                  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                  if (r > pkeep) {
                    k++;
                  }
                }

                // Use SBS to pick diverse points to not drop out
                std::vector<bool> keep_idx(N, false);
                
                // First, we start from a random point
                int j = rand() % N;
                keep_idx[j] = true;
                if (this->wmat_.shape[1] == 800) {
                  unit_hist_layer1[j] += 1;
                } else {
                  unit_hist_layer2[j] += 1;
                }

                // This vector will store the unnormalized values, initialize it
                // with the random element.
                std::vector<real_t> p_unnorm(N, false);
                for (int i=0; i<N; i++) {
                  p_unnorm[i] = D_[i][j];
                }

                // This is working space where we will store the normalized
                // distribution
                std::vector<real_t> p_norm(N, 0.0);
                std::vector<real_t> p_cdf(N, 0.0);

                int count = 1;
                // Now we sample the rest of the k-1 points
                for (int i=0; i<k-1; i++) {
                  // Generate the current distribution
                  real_t norm_factor = logsumexp(p_unnorm);
                  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

                  int idx = -1;
                  for (int j=0; j<p_unnorm.size(); j++) {
                    idx = j;

                    p_norm[j] = exp(p_unnorm[j] - norm_factor);

                    if (j==0) {
                      p_cdf[j] = p_norm[j];
                    } else {
                      p_cdf[j] = p_norm[j] + p_cdf[j-1];
                    }

                    if (r <= p_cdf[j]) {
                      break;
                    }
                  }

                  // If we haven't picked this, update, if not, ignore and
                  // continue
                  if (! keep_idx[idx]) {
                    keep_idx[idx] = true;
                    count++;

                    if (this->wmat_.shape[1] == 800) {
                      unit_hist_layer1[idx] += 1;
                    } else {
                      unit_hist_layer2[idx] += 1;
                    }

                    for (int j=0; j<p_unnorm.size(); j++) {
                      p_unnorm[j] += D_[j][idx];
                    }
                  }
                }

                // Pick random units if we don't match k
                if (count < k) {
                  for (int i=count; i<k; i++) {
                    int j = rand() % N;
                    while (keep_idx[j]) {
                      j = rand() % N;
                    }
                    keep_idx[j] = true;

                    if (this->wmat_.shape[1] == 800) {
                      unit_hist_layer1[j] += 1;
                    } else {
                      unit_hist_layer2[j] += 1;
                    }
                    
                  }
                }

                for (unsigned int i=0; i<mask_.shape[0]; i++) {
                  for (unsigned int j=0; j<mask_.shape[1]; j++) {
                    if (keep_idx[i]) {
                      mask_[0][0][j][i]  = 1.0f/pkeep;
                    } else {
                      mask_[0][0][j][i]  = 0.0;
                    }
                  }
                }

                this->in_.data = this->in_.data * mask_;
            }


            Parent::Forward( this->wmat_ );
        }
        virtual void Backprop(bool prop_grad) {
            Parent::Backprop( prop_grad, this->wmat_ );
            if (prop_grad) {
                this->in_.data *= mask_;
            }
        }
        virtual void InitLayer( void ){
            Parent::InitLayer();
            mask_.Resize( this->in_.data.shape );
            L_.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
            D_.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
            p.Resize(mshadow::Shape1(D_.shape[0]));

            this->w_normed.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
            p_unnorm.Resize(mshadow::Shape1(D_.shape[0]));

            counter = 0;

            for (int i=0; i<800; i++) {
              unit_hist_layer1.push_back(0);
              unit_hist_layer2.push_back(0);
            }

        }
    private:
        typedef FullConnectLayer<xpu> Parent;
    private:
        mshadow::TensorContainer<xpu, 4> mask_;
        mshadow::TensorContainer<xpu, 2> L_, D_;
        mshadow::TensorContainer<xpu, 2> w_normed;
        mshadow::TensorContainer<xpu, 1> p, p_unnorm;
        std::vector<int> unit_hist_layer1;
        std::vector<int> unit_hist_layer2;
        int counter;
    }; // class SimBasedDropinLayer
    
    template<typename xpu>
    class SimBasedDropoutLayer : public FullConnectLayer<xpu> {
    public:
        SimBasedDropoutLayer(mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out)
            : Parent(rnd, in, out) {}

        struct elemwise_log {
          MSHADOW_XINLINE static real_t Map(real_t a) {
            if (a < 1E-8) {
              return -18.0; // Approximately log(1E-8), a form of Lapacian smoothing
            } else {
              return log(a);
            }
          }
        };
        struct elemwise_sqrt {
          MSHADOW_XINLINE static real_t Map(real_t a) {
            return sqrt(a);
          }
        };
        struct elemwise_square {
          MSHADOW_XINLINE static real_t Map(real_t a) {
            return a*a;
          }
        };

        real_t logsumexp(std::vector<real_t> v) {
          // Find max
          real_t mx = v[0];
          for (int i=1; i<v.size(); i++) {
            if (v[i] > mx) {
              mx = v[i];
            }
          }

          real_t s = 0.0;
          // Log-sum-exp trick
          for (int i=0; i<v.size(); i++) {
            s += exp(v[i] - mx);
          }

          real_t lse = mx + log(s);

          return lse;
        }

        virtual void Forward(bool is_train) {
            if( is_train ){
                const real_t pkeep = 1.0f - Parent::param_.dropout_threshold;

                mask_ = F<op::threshold>( Parent::rnd_.uniform( mask_.shape ), pkeep ) * (1.0f/pkeep);
                int N = mask_.shape[0];

                // BEGIN SIM-BASED SAMPLING

                // First we normalize all the vectors
                w_normed = this->wmat_+0.0;
                for (unsigned int j=0; j<this->wmat_.shape[0]; j++) {
                  real_t w_normalizer = 0.0;
                  for (unsigned int i=0; i<this->wmat_.shape[1]; i++) {
                    w_normalizer += this->wmat_[i][j]*this->wmat_[i][j];
                  }

                  w_normalizer = sqrt(w_normalizer);
                  
                  for (unsigned int i=0; i<this->wmat_.shape[1]; i++) {
                    w_normed[i][j] = w_normed[i][j]/w_normalizer;
                  }
                }

                L_ = dot(w_normed.T(), w_normed);
                D_ =  (1 - L_ * L_);
                D_ = F<elemwise_log>(D_);

                std::ofstream Dfile;
                Dfile.open("D_cpu.txt");
                for (unsigned int j=0; j<this->D_.shape[0]; j++) {
                  for (unsigned int i=0; i<this->D_.shape[1]; i++) {
                    Dfile << this->D_[i][j] << ",";
                  }
                  Dfile << "\n";
                }
                Dfile.flush();
                Dfile.close();
                exit(0);

                // Sample k from a binomial distribution
                // Not fancy but works for now
                int k=0;
                for (int i=0; i<N; i++) {
                  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                  if (r > pkeep) {
                    k++;
                  }
                }

                // Use SBS to pick diverse points to not drop out
                std::vector<bool> keep_idx(N, false);
                
                // First, we start from a random point
                int j = rand() % N;
                keep_idx[j] = true;
                //if (this->wmat_.shape[1] == 500) {
                  //unit_hist_layer1[j] += 1;
                //} else {
                  //unit_hist_layer2[j] += 1;
                //}

                // This vector will store the unnormalized values, initialize it
                // with the random element.
                std::vector<real_t> p_unnorm(N, false);
                for (int i=0; i<N; i++) {
                  p_unnorm[i] = D_[i][j];
                }

                // This is working space where we will store the normalized
                // distribution
                std::vector<real_t> p_norm(N, 0.0);
                std::vector<real_t> p_cdf(N, 0.0);

                int count = 1;
                // Now we sample the rest of the k-1 points
                for (int i=0; i<k; i++) {
                  // Generate the current distribution
                  real_t norm_factor = logsumexp(p_unnorm);
                  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

                  int idx = -1;
                  for (int j=0; j<p_unnorm.size(); j++) {
                    idx = j;

                    p_norm[j] = exp(p_unnorm[j] - norm_factor);

                    if (j==0) {
                      p_cdf[j] = p_norm[j];
                    } else {
                      p_cdf[j] = p_norm[j] + p_cdf[j-1];
                    }

                    if (r <= p_cdf[j]) {
                      break;
                    }
                  }

                  // If we haven't picked this, update, if not, ignore and
                  // continue
                  if (! keep_idx[idx]) {
                    keep_idx[idx] = true;
                    count++;

                    //if (this->wmat_.shape[1] == 500) {
                      //unit_hist_layer1[idx] += 1;
                    //} else {
                      //unit_hist_layer2[idx] += 1;
                    //}

                    for (int j=0; j<p_unnorm.size(); j++) {
                      p_unnorm[j] += D_[j][idx];
                    }
                  }
                }

                if (this->wmat_.shape[1] == 500) {
                  k_vals_layer1.push_back(k);
                  count_vals_layer1.push_back(count);
                } else {
                  k_vals_layer2.push_back(k);
                  count_vals_layer2.push_back(count);
                }

                // Pick random units if we don't match k
                if (count < k) {
                  for (int i=count; i<k; i++) {
                    int j = rand() % N;
                    while (keep_idx[j]) {
                      j = rand() % N;
                    }
                    keep_idx[j] = true;

                    //if (this->wmat_.shape[1] == 500) {
                      //unit_hist_layer1[j] += 1;
                    //} else {
                      //unit_hist_layer2[j] += 1;
                    //}
                    
                  }
                }

                for (unsigned int i=0; i<mask_.shape[0]; i++) {
                  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                  for (unsigned int j=0; j<mask_.shape[1]; j++) {
                    //if (r > pkeep) {
                    if (!keep_idx[i]) {
                      mask_[0][0][j][i]  = 1.0f/pkeep;
                    } else {
                      mask_[0][0][j][i]  = 0.0;
                    }
                  }
                }


                //counter++;
                //if (counter%(600*50)==0) {
                  //if (this->wmat_.shape[1] == 500) {
                    //std::ofstream hist1file;
                    //hist1file.open("hist1.txt");
                    //for (unsigned int j=0; j<unit_hist_layer1.size(); j++) {
                      //hist1file << unit_hist_layer1[j] << "\n";
                    //}
                    //hist1file.flush();
                    //hist1file.close();

                    //std::ofstream kvals_file;
                    //kvals_file.open("kvals1.txt");
                    //for (unsigned int j=0; j<k_vals_layer1.size(); j++) {
                      //kvals_file << k_vals_layer1[j] << "\n";
                    //}
                    //kvals_file.flush();
                    //kvals_file.close();

                    //std::ofstream countvals_file;
                    //countvals_file.open("countvals1.txt");
                    //for (unsigned int j=0; j<count_vals_layer1.size(); j++) {
                      //countvals_file << count_vals_layer1[j] << "\n";
                    //}
                    //countvals_file.flush();
                    //countvals_file.close();

                    //std::ofstream wmatfile;
                    //wmatfile.open("wmat1.txt");
                    //for (unsigned int j=0; j<this->wmat_.shape[0]; j++) {
                      //for (unsigned int i=0; i<this->wmat_.shape[1]; i++) {
                        //wmatfile << this->wmat_[i][j] << ",";
                      //}
                      //wmatfile << "\n";
                    //}
                    //wmatfile.flush();
                    //wmatfile.close();
                  //} else {
                    //std::ofstream hist2file;
                    //hist2file.open("hist2.txt");
                    //for (unsigned int j=0; j<unit_hist_layer2.size(); j++) {
                      //hist2file << unit_hist_layer2[j] << "\n";
                    //}
                    //hist2file.flush();
                    //hist2file.close();

                    //std::ofstream kvals_file;
                    //kvals_file.open("kvals2.txt");
                    //for (unsigned int j=0; j<k_vals_layer2.size(); j++) {
                      //kvals_file << k_vals_layer2[j] << "\n";
                    //}
                    //kvals_file.flush();
                    //kvals_file.close();

                    //std::ofstream countvals_file;
                    //countvals_file.open("countvals2.txt");
                    //for (unsigned int j=0; j<count_vals_layer2.size(); j++) {
                      //countvals_file << count_vals_layer2[j] << "\n";
                    //}
                    //countvals_file.flush();
                    //countvals_file.close();

                    //std::ofstream wmatfile;
                    //wmatfile.open("wmat2.txt");
                    //for (unsigned int j=0; j<this->wmat_.shape[0]; j++) {
                      //for (unsigned int i=0; i<this->wmat_.shape[1]; i++) {
                        //wmatfile << this->wmat_[i][j] << ",";
                      //}
                      //wmatfile << "\n";
                    //}
                    //wmatfile.flush();
                    //wmatfile.close();
                    //exit(0);
                  //}
                //}



                  //std::ofstream p_unnormfile;
                  //p_unnormfile.open("p_unnorm.txt");
                  //for (unsigned int j=0; j<p_unnorm.size(); j++) {
                    //p_unnormfile << p_unnorm[j] << "\n";
                  //}
                  //p_unnormfile.flush();
                  //p_unnormfile.close();

                  ////std::cout << "LSE is: " << logsumexp(p_unnorm) << std::endl;

                  //exit(0);
                //}
                  //std::ofstream wmatfile;
                  //wmatfile.open("wmat.txt");
                  //for (unsigned int j=0; j<this->wmat_.shape[0]; j++) {
                    //for (unsigned int i=0; i<this->wmat_.shape[1]; i++) {
                      //wmatfile << this->wmat_[i][j] << ",";
                    //}
                    //wmatfile << "\n";
                  //}
                  //wmatfile.flush();
                  //wmatfile.close();

                  ////std::ofstream wmatnormedfile;
                  ////wmatnormedfile.open("wmatnormed.txt");
                  ////for (unsigned int j=0; j<w_normed.shape[0]; j++) {
                    ////for (unsigned int i=0; i<w_normed.shape[1]; i++) {
                      ////wmatnormedfile << w_normed[i][j] << ",";
                    ////}
                    ////wmatnormedfile << "\n";
                  ////}
                  ////wmatnormedfile.flush();
                  ////wmatnormedfile.close();

                  //std::ofstream Lfile, Dfile;
                  //Lfile.open("L.txt");
                  //for (unsigned int j=0; j<L_.shape[0]; j++) {
                    //for (unsigned int i=0; i<L_.shape[1]; i++) {
                      //Lfile << L_[i][j] << ",";
                    //}
                    //Lfile << "\n";
                  //}
                  //Lfile.flush();
                  //Lfile.close();

                  //Dfile.open("D.txt");
                  //for (unsigned int j=0; j<D_.shape[0]; j++) {
                    //for (unsigned int i=0; i<D_.shape[1]; i++) {
                      //Dfile << D_[i][j] << ",";
                    //}
                    //Dfile << "\n";
                  //}
                  //Dfile.flush();
                  //Dfile.close();

                  //exit(1);
                //}

                this->in_.data = this->in_.data * mask_;

            }


            Parent::Forward( this->wmat_ );
        }
        virtual void Backprop(bool prop_grad) {
            Parent::Backprop( prop_grad, this->wmat_ );
            if (prop_grad) {
                this->in_.data *= mask_;
            }
        }
        virtual void InitLayer( void ){
            Parent::InitLayer();
            mask_.Resize( this->in_.data.shape );
            L_.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
            D_.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
            p.Resize(mshadow::Shape1(D_.shape[0]));

            this->w_normed.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
            p_unnorm.Resize(mshadow::Shape1(D_.shape[0]));

            counter = 0;

            for (int i=0; i<500; i++) {
              unit_hist_layer1.push_back(0);
              unit_hist_layer2.push_back(0);
            }

        }
    private:
        typedef FullConnectLayer<xpu> Parent;
    private:
        mshadow::TensorContainer<xpu, 4> mask_;
        mshadow::TensorContainer<xpu, 2> L_, D_;
        mshadow::TensorContainer<xpu, 2> w_normed;
        mshadow::TensorContainer<xpu, 1> p, p_unnorm;
        std::vector<int> unit_hist_layer1;
        std::vector<int> unit_hist_layer2;
        std::vector<int> k_vals_layer1;
        std::vector<int> k_vals_layer2;
        std::vector<int> count_vals_layer1;
        std::vector<int> count_vals_layer2;
        int counter;
    }; // class SimBasedDropoutLayer

    template<typename xpu>
    class SimBasedDropoutGPULayer : public FullConnectLayer<xpu> {
    public:
        SimBasedDropoutGPULayer(mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out)
            : Parent(rnd, in, out) {}

        struct elemwise_log {
          MSHADOW_XINLINE static real_t Map(real_t a) {
            if (a < 1E-8) {
              return -18.0; // Approximately log(1E-8), a form of Lapacian smoothing
            } else {
              return log(a);
            }
          }
        };
        struct elemwise_sqrt {
          MSHADOW_XINLINE static real_t Map(real_t a) {
            return sqrt(a);
          }
        };
        struct elemwise_square {
          MSHADOW_XINLINE static real_t Map(real_t a) {
            return a*a;
          }
        };
        struct elemwise_divide {
          MSHADOW_XINLINE static real_t Map( real_t a, real_t b) {
            return a/b;
          }
        };

        real_t logsumexp(std::vector<real_t> v) {
          // Find max
          real_t mx = v[0];
          for (int i=1; i<v.size(); i++) {
            if (v[i] > mx) {
              mx = v[i];
            }
          }

          real_t s = 0.0;
          // Log-sum-exp trick
          for (int i=0; i<v.size(); i++) {
            s += exp(v[i] - mx);
          }

          real_t lse = mx + log(s);

          return lse;
        }

        virtual void Forward(bool is_train) {
            if( is_train ){
                const real_t pkeep = 1.0f - Parent::param_.dropout_threshold;

                mask_ = F<op::threshold>( Parent::rnd_.uniform( mask_.shape ), pkeep ) * (1.0f/pkeep);
                int N = mask_.shape[0];

                // First we normalize all the vectors
                Copy(w_normed, this->wmat_);
                w_normed = F<elemwise_square>(w_normed);
                w_normalizer = sumall_except_dim<0>(w_normed);
                w_normalizer = F<elemwise_sqrt>(w_normalizer);

                // This is a bit weird that we're broadcasting along dim. 0, but
                // mshadow orders dimensions based on size, not rows/cols
                w_normalizer_repmat = broadcast<0>(w_normalizer, w_normalizer_repmat.shape);
                w_normed = F<elemwise_divide>(this->wmat_, w_normalizer_repmat);

                L_ = dot(w_normed.T(), w_normed);
                D_ =  (1 - L_ * L_);
                D_ = F<elemwise_log>(D_);
                Copy(D_host, D_);

                // Sample k from a binomial distribution
                // Not fancy but works for now
                int k=0;
                for (int i=0; i<N; i++) {
                  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                  if (r > pkeep) {
                    k++;
                  }
                }

                // Use SBS to pick diverse points to not drop out
                std::vector<bool> keep_idx(N, false);
                
                // First, we start from a random point
                int j = rand() % N;
                keep_idx[j] = true;

                // This vector will store the unnormalized values, initialize it
                // with the random element.
                std::vector<real_t> p_unnorm(N, false);
                for (int i=0; i<N; i++) {
                  p_unnorm[i] = D_host[i][j];
                }

                // This is working space where we will store the normalized
                // distribution
                std::vector<real_t> p_norm(N, 0.0);
                std::vector<real_t> p_cdf(N, 0.0);

                int count = 1;
                // Now we sample the rest of the k-1 points
                for (int i=0; i<k; i++) {
                  // Generate the current distribution
                  real_t norm_factor = logsumexp(p_unnorm);
                  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

                  int idx = -1;
                  for (int j=0; j<p_unnorm.size(); j++) {
                    idx = j;

                    p_norm[j] = exp(p_unnorm[j] - norm_factor);

                    if (j==0) {
                      p_cdf[j] = p_norm[j];
                    } else {
                      p_cdf[j] = p_norm[j] + p_cdf[j-1];
                    }

                    if (r <= p_cdf[j]) {
                      break;
                    }
                  }

                  // If we haven't picked this, update, if not, ignore and
                  // continue
                  if (! keep_idx[idx]) {
                    keep_idx[idx] = true;
                    count++;

                    for (int j=0; j<p_unnorm.size(); j++) {
                      p_unnorm[j] += D_host[j][idx];
                    }
                  }
                }

                // Pick random units if we don't match k
                if (count < k) {
                  for (int i=count; i<k; i++) {
                    int j = rand() % N;
                    while (keep_idx[j]) {
                      j = rand() % N;
                    }
                    keep_idx[j] = true;
                    
                  }
                }

                for (unsigned int i=0; i<mask_host.shape[0]; i++) {
                  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                  for (unsigned int j=0; j<mask_host.shape[1]; j++) {
                    //if (r > pkeep) {
                    if (!keep_idx[i]) {
                      mask_host[0][0][j][i]  = 1.0f/pkeep;
                    } else {
                      mask_host[0][0][j][i]  = 0.0;
                    }
                  }
                }

                Copy(mask_, mask_host);
                this->in_.data = this->in_.data * mask_;

            }


            Parent::Forward( this->wmat_ );
        }
        virtual void Backprop(bool prop_grad) {
            Parent::Backprop( prop_grad, this->wmat_ );
            if (prop_grad) {
                this->in_.data *= mask_;
            }
        }
        virtual void InitLayer( void ){
            Parent::InitLayer();
            mask_.Resize( this->in_.data.shape );
            mask_host.Resize( this->in_.data.shape );
            L_.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
            D_.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
            D_host.Resize( mshadow::Shape2( this->in_.data.shape[0], this->in_.data.shape[0] ) );
            p.Resize(mshadow::Shape1(D_.shape[0]));

            this->w_normed.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
            this->w_normalizer.Resize( mshadow::Shape1( this->in_.data.shape[0] ) );
            this->w_normalizer_repmat.Resize( mshadow::Shape2( this->out_.data.shape[0], this->in_.data.shape[0] ) );
            p_unnorm.Resize(mshadow::Shape1(D_.shape[0]));

            counter = 0;
        }
    private:
        typedef FullConnectLayer<xpu> Parent;
    private:
        mshadow::TensorContainer<xpu, 4> mask_;
        mshadow::TensorContainer<cpu, 4> mask_host;
        mshadow::TensorContainer<xpu, 2> L_, D_;
        mshadow::TensorContainer<cpu, 2> D_host;
        mshadow::TensorContainer<xpu, 2> w_normed;
        mshadow::TensorContainer<xpu, 1> w_normalizer;
        mshadow::TensorContainer<xpu, 2> w_normalizer_repmat;
        mshadow::TensorContainer<xpu, 1> p, p_unnorm;
        int counter;
    }; // class SimBasedDropoutGPULayer


    template<typename xpu>
    class MyDropoutLayer : public FullConnectLayer<xpu> {
    public:
        MyDropoutLayer(mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out)
            : Parent(rnd, in, out) {}


        virtual void Forward(bool is_train) {
            if( is_train ){
                const real_t pkeep = 1.0f - Parent::param_.dropout_threshold;

                mask_ = F<op::threshold>( Parent::rnd_.uniform( mask_.shape ), pkeep ) * (1.0f/pkeep);
                int N = mask_.shape[0];

                // First we normalize all the vectors
                for (unsigned int i=0; i<mask_.shape[0]; i++) {
                  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                  for (unsigned int j=0; j<mask_.shape[1]; j++) {
                    if (r > pkeep) {
                      mask_[0][0][j][i]  = 1.0f/pkeep;
                    } else {
                      mask_[0][0][j][i]  = 0.0;
                    }
                  }
                }
                this->in_.data = this->in_.data * mask_;
            }

            Parent::Forward( this->wmat_ );
        }
        virtual void Backprop(bool prop_grad) {
            Parent::Backprop( prop_grad, this->wmat_ );
            if (prop_grad) {
                this->in_.data *= mask_;
            }
        }
        virtual void InitLayer( void ){
            Parent::InitLayer();
            mask_.Resize( this->in_.data.shape );

        }
    private:
        typedef FullConnectLayer<xpu> Parent;
    private:
        mshadow::TensorContainer<xpu, 4> mask_;
    }; // class MyDropoutLayer

    template<typename xpu>
    class MyDropoutPerMbatchLayer : public FullConnectLayer<xpu> {
    public:
        MyDropoutPerMbatchLayer(mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out)
            : Parent(rnd, in, out) {}

        virtual void Forward(bool is_train) {
            if( is_train ){
                const real_t pkeep = 1.0f - Parent::param_.dropout_threshold;

                mask_ = F<op::threshold>( Parent::rnd_.uniform( mask_.shape ), pkeep ) * (1.0f/pkeep);
                int N = mask_.shape[0];

                // First we normalize all the vectors
                for (unsigned int i=0; i<mask_.shape[0]; i++) {
                  for (unsigned int j=0; j<mask_.shape[1]; j++) {
                    float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                    if (r > pkeep) {
                      mask_[0][0][j][i]  = 1.0f/pkeep;
                    } else {
                      mask_[0][0][j][i]  = 0.0;
                    }
                  }
                }
                this->in_.data = this->in_.data * mask_;
            }

            Parent::Forward( this->wmat_ );
        }
        virtual void Backprop(bool prop_grad) {
            Parent::Backprop( prop_grad, this->wmat_ );
            if (prop_grad) {
                this->in_.data *= mask_;
            }
        }
        virtual void InitLayer( void ){
            Parent::InitLayer();
            mask_.Resize( this->in_.data.shape );
        }
    private:
        typedef FullConnectLayer<xpu> Parent;
    private:
        mshadow::TensorContainer<xpu, 4> mask_;
    }; // class MyDropoutPerMbatchLayer

    template<typename xpu>
    class DropoutLayer : public ILayer {
    public:
        DropoutLayer(mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out)
            :rnd_(rnd), out_(out) {
            Assert( &in == &out, "dropout layer must self loop e.g layer[1->1] = dropout" );
        }
        virtual void SetParam(const char *name, const char* val){
            param_.SetParam( name, val );
        }
        virtual void Forward( bool is_train ) {
            if (is_train) {
                // BDOL-NOTE: Here is where dropout is
                const real_t pkeep = 1.0f - param_.dropout_threshold;
                mask_ = F<op::threshold>( rnd_.uniform( mask_.shape ), pkeep ) * (1.0f/pkeep);
                out_.data = out_.data * mask_;
            }
        }
        virtual void Backprop( bool prop_grad ) {
            if (prop_grad) {
                out_.data *= mask_;
            }
        }
        virtual void InitLayer( void ) {
            utils::Assert(param_.dropout_threshold >= 0.0f && param_.dropout_threshold < 1.0f, "DropoutLayer: invalid dropout threshold\n");
            mask_.Resize( out_.data.shape );
        }
    private:
        /*! \brief random number generator */
        mshadow::Random<xpu> &rnd_;
        /*! \brief output node */
        Node<xpu> &out_;
        /*! \brief dropout mask */
        mshadow::TensorContainer<xpu, 4> mask_;
        /*! \brief parameters that potentially be useful */
        LayerParam param_;
    }; // class DropoutLayer
}; // namespace cxxnet

namespace cxxnet{
    /* layer patch that handles memory issues */
    template<typename xpu>
    struct LayerPatch: public ILayer{
    public:
        LayerPatch( ILayer *base, Node<xpu>& in, Node<xpu>& out )
            :base_(base), in_(in), out_(out){}
        virtual ~LayerPatch( void ){ delete base_; }
        virtual void Forward( bool is_train ){
            in_.Pin(); out_.Pin();
            base_->Forward( is_train );
            in_.Unpin(); out_.Unpin();
        }
        virtual void Backprop( bool prop_grad ){
            in_.Pin(); out_.Pin();
            base_->Backprop( prop_grad );
            in_.Unpin(); out_.Unpin();
        }
    public:
        virtual void InitLayer( void ){
            base_->InitLayer();
        }
        virtual void GetUpdaters( const char *updater, std::vector<IUpdater*> &updaters ) {
            base_->GetUpdaters( updater, updaters );
        }
        virtual void SetParam( const char *name, const char* val ) {
            base_->SetParam( name, val );
        }
        virtual void InitModel(void) {
            base_->InitModel();
        }
        virtual void SaveModel(mshadow::utils::IStream &fo) const {
            base_->SaveModel( fo );
        }
        virtual void LoadModel(mshadow::utils::IStream &fi) {
            base_->LoadModel( fi );
        }
    private:
        ILayer *base_;
        Node<xpu> &in_, &out_;
    };
};

namespace cxxnet{
    inline int GetLayerType( const char *type ){
        if( !strncmp( type, "pairtest-", 9 ) ){
            char tmaster[256], tslave[256];
            sscanf( type + 9, "%[^-]-%[^:]", tmaster, tslave );
            return GetLayerType(tmaster) * 1000  + GetLayerType(tslave);
        }
        using namespace layer_type;
        if( !strcmp( type, "fullc") )   return kFullConnect;
        if( !strcmp( type, "softmax") ) return kSoftmax;
        if( !strcmp( type, "relu") ) return kRectifiedLinear;
        if( !strcmp( type, "sigmoid") ) return kSigmoid;
        if( !strcmp( type, "tanh") ) return kTanh;
        if( !strcmp( type, "softplus") ) return kSoftplus;
        if( !strcmp( type, "flatten") )  return kFlatten;
        if( !strcmp( type, "dropout") ) return kDropout;
        if( !strcmp( type, "dropconn") ) return kDropConn;
        if( !strcmp( type, "dpp_dropout") ) return kDppDropout;
        if( !strcmp( type, "dpp_dropin") ) return kDppDropin;
        if( !strcmp( type, "strata") ) return kStrata;
        if( !strcmp( type, "simbased_dropout") ) return kSimBasedDropout;
        if( !strcmp( type, "simbased_dropout_gpu") ) return kSimBasedDropoutGPU;
        if( !strcmp( type, "simbased_dropin") ) return kSimBasedDropin;
        if( !strcmp( type, "my_dropout") ) return kMyDropout;
        if( !strcmp( type, "my_dropout_permbatch") ) return kMyDropoutPerMbatch;
        if( !strcmp( type, "dpp_dropout_onlyquality") ) return kDppDropoutOnlyQuality;
        //if( !strcmp( type, "dpp_dropin_onlyquality") ) return kDppDropinOnlyQuality;
        if( !strcmp( type, "dpp_dropout_onlysimilarity") ) return kDppDropoutOnlySimilarity;
        if( !strcmp( type, "dpp_dropin_onlysimilarity") ) return kDppDropinOnlySimilarity;
        if( !strcmp( type, "conv") )     return kConv;
        if( !strcmp( type, "max_pooling")) return kMaxPooling;
        if( !strcmp( type, "sum_pooling")) return kSumPooling;
        if( !strcmp( type, "avg_pooling")) return kAvgPooling;
        if( !strcmp( type, "padding"))   return kPadding;
        if( !strcmp( type, "lrn"))       return kLRN;
        if( !strcmp( type, "caffe") )    return kCaffe;
        fprintf(stderr, "unknown layer type: %s\n", type);
        Error("unknown layer type" );
        return 0;
    }

    template<typename xpu>
    inline ILayer* CreateLayer_( int type, mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out ){
        using namespace layer_type;
        if( type >= 1000 ){
            return new PairTestLayer<xpu>( rnd, in, out, type/1000, type % 1000);
        }
        switch( type ){
        case kFullConnect: return new FullConnectLayer<xpu>( rnd, in, out );
        case kSoftmax    : return new SoftmaxLayer<xpu>( rnd, in, out );
        case kSigmoid : return new ActivationLayer<xpu,op::sigmoid,op::sigmoid_grad>(in, out);
        case kTanh    : return new ActivationLayer<xpu,op::tanh,op::tanh_grad>(in, out);
        case kRectifiedLinear: return new ActivationLayer<xpu,op::relu,op::relu_grad>(in, out);
        case kSoftplus: return new ActivationLayer<xpu,op::softplus,op::softplus_grad>(in, out);
        case kFlatten:  return new FlattenLayer<xpu>( in, out );
        case kDropConn: return new DropConnLayer<xpu>(rnd, in, out);
        case kDropout: return new DropoutLayer<xpu>(rnd, in, out);
        case kDppDropout: return new DppDropoutLayer<xpu>(rnd, in, out);
        case kDppDropin: return new DppDropinLayer<xpu>(rnd, in, out);
        case kStrata: return new StratifiedSamplingLayer<xpu>(rnd, in, out);
        case kSimBasedDropout: return new SimBasedDropoutLayer<xpu>(rnd, in, out);
        case kSimBasedDropoutGPU: return new SimBasedDropoutGPULayer<xpu>(rnd, in, out);
        case kSimBasedDropin: return new SimBasedDropinLayer<xpu>(rnd, in, out);
        case kMyDropout: return new MyDropoutLayer<xpu>(rnd, in, out);
        case kMyDropoutPerMbatch: return new MyDropoutPerMbatchLayer<xpu>(rnd, in, out);
        case kDppDropoutOnlyQuality: return new DppDropoutOnlyQualityLayer<xpu>(rnd, in, out);
        //case kDppDropinOnlyQuality: return new DppDropinOnlyQualityLayer<xpu>(rnd, in, out);
        case kDppDropoutOnlySimilarity: return new DppDropoutOnlySimilarityLayer<xpu>(rnd, in, out);
        case kDppDropinOnlySimilarity: return new DppDropinOnlySimilarityLayer<xpu>(rnd, in, out);
        case kConv:    return new ConvolutionLayer<xpu>( rnd, in, out );
        case kMaxPooling: return new PoolingLayer<mshadow::red::maximum, false, xpu>(in, out);
        case kSumPooling: return new PoolingLayer<mshadow::red::sum, false, xpu>(in, out);
        case kAvgPooling: return new PoolingLayer<mshadow::red::sum, true, xpu>(in, out);
        case kPadding: return new PaddingLayer<xpu>(in, out);
        case kLRN:     return new LRNLayer<xpu>(in, out);
#if CXXNET_ADAPT_CAFFE
        case kCaffe: return new CaffeLayer<xpu>(rnd,in,out);
#endif
        default: Error("unknown layer type");
        }
        return NULL;
    }
    template<typename xpu>
    inline ILayer* CreateLayer( int type, mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out ){
        return new LayerPatch<xpu>( CreateLayer_<xpu>(type,rnd,in,out), in, out );
    }
    template<typename xpu>
    inline ILayer* CreateLayer( const char *type, mshadow::Random<xpu> &rnd, Node<xpu> &in, Node<xpu> &out ){
        return CreateLayer( GetLayerType(type), rnd, in, out );
    }
};

#endif // CXXNET_LAYER_INL_HPP
