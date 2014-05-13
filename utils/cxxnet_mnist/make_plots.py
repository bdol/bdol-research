import ConfigParser
import argparse
import ast
import itertools, os, shutil
import numpy as np
import sys
import time
import matplotlib.pyplot as plt

def paramsToString(layerSize, activation, dropoutSetting):
  s = ""
  for i in range(1, len(layerSize)-1):
    s += str(layerSize[i])+"_"
  s += str(activation)+"_"
  s += str(dropoutSetting)

  return s

class ExpResult:
  def __init__(self, layerSize):
    self.layerSize = layerSize
    self.results = []

class ExpData:
  def __init__(self, data, name):
    self.data = data
    self.name = name

if __name__=="__main__":
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
  
  expTopLevelDir = os.path.join(os.path.abspath(args.destination), expName)

  N = len(activations)
  res = ExpResult(layerSizes)
  print "Getting data..."
  for i in range(0, N):
    expStr = "{0}_{1}".format(activations[i], dropoutSettings[i])
    expDir = os.path.join(expTopLevelDir, expStr)
    f = open(os.path.join(expDir, "results.log"), "r")
    r = []
    # Just parse each line of the file to get to the test error
    for line in f:
      tok = line.split(":")
      if len(tok) > 1:
        tok = tok[1].split("\t")
        r.append(tok[0])

    f.close()

    data = ExpData(r, expStr)
    res.results.append(data)
    
  figCounter = 0
  e = res
  figCounter += 1
  plt.figure(figCounter)
  plt.gca().set_color_cycle(['red', 'blue', 'green', 'yellow', 'black', 'magenta',
    'cyan'])

  # First find the extent of the data
  maxLen = 0
  for d in e.results:
    if len(d.data)>maxLen:
      maxLen = len(d.data)

  xx = np.arange(0, maxLen)
  for d in e.results:
    plt.plot(xx[0:len(d.data)], d.data, label=d.name)
  
  plt.title("Layer config: "+str(e.layerSize))
  plt.xlabel("Epochs")
  plt.ylabel("Test errors")
  plt.legend()
  plt.show()

  sys.exit(0)
  

  # All of the experiment directories
  cfg = ConfigParser.ConfigParser()
  cfg.read(sys.argv[2])
  layerSizes = list(ast.literal_eval(cfg.get('exps', 'layerSizes')))
  activations = cfg.get('exps', 'activations').split(',')
  activations = [item.strip() for item in activations]
  dropoutSettings = cfg.get('exps', 'dropoutSettings').split(',')
  dropoutSettings = [item.strip() for item in dropoutSettings]

  maxEpochs = []
  maxE = 0
  eCounter = 0
  resultsList = []
  prevLayerSize = None
  print layerSizes

  for i in range(0, len(layerSizes)):
    if layerSizes[i]!=prevLayerSize: #We're on a new network configuration
      resultsList.append(ExpResult(layerSizes[i]))
      prevLayerSize = layerSizes[i]
      print "Reading data for configuration", layerSizes[i]


    expStr = paramsToString(layerSizes[i], activations[i], dropoutSettings[i])
    expDir = os.path.join(experiments_dir, expStr)
    for file in os.listdir(os.path.join(expDir, 'logfiles')):
      if file.endswith(".txt"):
        data=np.genfromtxt(os.path.join(expDir, 'logfiles', file), delimiter=',')
        expName = activations[i]+", "+dropoutSettings[i]
        expData = ExpData(data[1:, 1], expName)
        resultsList[-1].results.append(expData)


  figCounter = 0
  for e in resultsList:
    figCounter += 1
    plt.figure(figCounter)
    plt.gca().set_color_cycle(['red', 'blue', 'green', 'yellow', 'black', 'magenta',
      'cyan'])
    lstyle = itertools.cycle(('-','--'))

    # First find the extent of the data
    maxLen = 0
    for d in e.results:
      if len(d.data)>maxLen:
        maxLen = len(d.data)

    xx = np.arange(0, maxLen)
    for d in e.results:
      plt.plot(xx[0:len(d.data)], d.data, label=d.name, linestyle=lstyle.next())
    
    plt.title("Layer config: "+str(e.layerSize))
    plt.xlabel("Epochs")
    plt.ylabel("Test errors")
    plt.legend()
    plt.show()
