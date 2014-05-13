import ConfigParser
import argparse
import ast
import itertools, os, shutil
import sys
import time

def cleanup():
  print "IMPLEMENT CLEANUP METHOD!!"

def create_net_config(layerSize, activation, dropoutType):
  s = "netconfig=start\n"

  # Add input dropout
  if dropoutType == "dropout":
    s += '''layer[0->0] = dropout
  threshold = 0.2\n'''

  c = 0
  for i in range(1, len(layerSize)):
    s += '''layer[{0}->{1}] = fullc:fc{0}
  nhidden = {2}
  init_sigma = 0.01\n'''.format(c, c+1, layerSize[i])
    c += 1

    if i != len(layerSize)-1:
      s += '''layer[{0}->{1}] = {2}:act{0}\n'''.format(c, c+1, activation)
      if dropoutType=="dropout":
        s += '''layer[{0}->{0}] = dropout
  threshold = 0.5\n'''.format(c+1)
    else:
      s += '''layer[{0}->{0}] = softmax:act{0}\n'''.format(c)
     
    c += 1

  return s

def params_to_file_contents(layerSize, activation, dropoutType, dataPath):
  '''
  Given the above parameters, this method generates the contents that will go
  into the MNIST.conf file.
  '''
  trainDataPath = os.path.join(dataPath, "train-images-idx3-ubyte.gz")
  trainLabelsPath = os.path.join(dataPath, "train-labels-idx1-ubyte.gz")
  testDataPath = os.path.join(dataPath, "t10k-images-idx3-ubyte.gz")
  testLabelsPath = os.path.join(dataPath, "t10k-labels-idx1-ubyte.gz")

  s = '''data = train
iter = mnist
    path_img = {0}
    path_label = {1}
    shuffle = 1
iter = end

eval = test
iter = mnist
    path_img = {2}
    path_label = {3}
iter = end\n\n'''.format(trainDataPath, trainLabelsPath,
                    testDataPath, testLabelsPath)

  s += create_net_config(layerSize, activation, dropoutType)
  s += "\n\n"

  s += '''input_shape = 1,1,784
batch_size = 100

dev = gpu
save_model = 15
max_round = 5000
num_round = 5000
train_eval = 1
random_type = gaussian

eta = 0.1
momentum = 0.9
wd  = 0.0

metric = error\n'''

  return s

if __name__=="__main__":
  # Models directory, log files directory
  parser = argparse.ArgumentParser()
  parser.add_argument("-p", "--params", type=str, 
      help="create experiment with the given experiments.ini file")
  parser.add_argument("-d", "--destination", type=str,
      help="the root destination for the experiment directory")
  parser.add_argument("-c", "--clean", action="store_true",
      help="clean up all experiment files")
  args = parser.parse_args()

  if not args.params:
    print "Error: No parameter file given! Use -p <params_file>."
    sys.exit(0)
  if args.params and not args.destination:
    print "Error: No destination given! Use -d <params_file>."
    sys.exit(0)

  if args.clean:
    print "Are you sure you want to delete ALL experiment files? [y/n]"
    choice = raw_input().lower()
    if choice == "y":
      print "Are you REALLY sure? [y/n]"
      choice = raw_input().lower()
      if choice == "y":
        cleanup()
        sys.exit(0)
    sys.exit(0)

  #dateStr = time.strftime('%Y-%m-%d_%H-%M')

  # All of the experiments to create code/results directories for
  cfg = ConfigParser.ConfigParser()
  cfg.read(args.params)
  expName = cfg.get('exps', 'name')
  layerSizes = list(ast.literal_eval(cfg.get('exps', 'layerSizes')))
  activations = cfg.get('exps', 'activations').split(',')
  activations = [item.strip() for item in activations]
  dropoutSettings = cfg.get('exps', 'dropoutSettings').split(',')
  dropoutSettings = [item.strip() for item in dropoutSettings]
  dataPath = os.path.abspath(cfg.get('exps', 'dataPath'))
  binPath = os.path.abspath(cfg.get('exps', 'binPath'))

  # First create the top-level experiment directory
  expTopLevelDir = os.path.join(os.path.abspath(args.destination), expName)
  print "Creating experiment {0} in {1}".format(expName, expTopLevelDir)
  if not os.path.isdir(expTopLevelDir):
    os.mkdir(expTopLevelDir)

  # Next create the head script that will submit all jobs to SGE
  f_sge_head = open("run_{0}.sh".format(expName), "w")



  N = len(activations)
  print 'Creating {0} experiment(s)...'.format(N)
  for i in range(0, N):
    # Create each experiment dir
    expStr = "{0}_{1}".format(activations[i], dropoutSettings[i])
    expDir = os.path.join(expTopLevelDir, expStr)
    print "Creating experiment {0}".format(expStr)

    # Create the MNIST.conf file used for input to the cxxnet bin
    if not os.path.isdir(expDir):
      os.mkdir(expDir)
    f_conf = open(os.path.join(expDir, "MNIST.conf"), "w")
    confContents = params_to_file_contents(layerSizes, activations[i],
        dropoutSettings[i], dataPath)
    f_conf.write(confContents)
    f_conf.close()

    # Create the folder for saved models
    if not os.path.isdir(os.path.join(expDir, "models")):
      os.mkdir(os.path.join(expDir, "models"))

    # Create the experiment's SGE script
    f_sge = open(os.path.join(expDir, expStr+"_"+expName+".sh"), "w")
    f_sge.write('''#!/bin/sh\n
#$ -V
#$ -cwd
#$ -S /bin/sh
#$ -m be
#$ -M bdolmail@gmail.com

{0} {1} 2>results.log
'''.format(binPath, os.path.join(expDir, "MNIST.conf")))

    f_sge.close()
    f_sge_head.write("qsub -q cuda.q "+os.path.join(expDir, expStr+"_"+expName+".sh")+"\n")

  f_sge_head.close()
  print "Done!"
