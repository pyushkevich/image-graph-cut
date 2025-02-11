#include "ImageGraphCut.h"
#include <iostream>

using namespace std;

int usage()
{
  const char *usage =
    "usage: metisseg [options] input.img output.img num_part"
    "\n   uses METIS to segment a binary image into num_part partitions"
    "\noptions: "
    "\n   -w X.X X.X          Specify relative weights of the partitions"
    "\n   -p N1 N2 N3         define cut plane at dimension N1, slice N2"
    "\n                       with relative edge strength N3"
    "\n   -o                  use optimization to refine partition weights"
    "\n   -u float            Load imbalance tolerance (ubvec in METIS). "
    "\n                       Must be >= 1. Larger values means more flexibility"
    "\n                       for non-equal partitions"
    "\n   -n number           Number of iterations of internal METIS optimization"
    "\n   -c N frac           Allow up to N connected components in the input image "
    "\n                       rejecting components smaller than frac of total foreground"
    "\n                       each component will be handled separately"
    "\nhint files: "
    "\n   The hint file is used to convert an image into a graph. It specifies "
    "\n   the weights assigned to the vertices and edges in the graph based on"
    "\n   the intensities of the pixels that connect the edges. The vertex "
    "\n   weight of zero means that the pixels with a given intensity are not"
    "\n   included in the graph. The hint file contains the following types of"
    "\n   entries."
    "\n   "
    "\n     V int wgt"
    "\n     E int1 int2 wgt"
    "\n   "
    "\n   The first entry specifies vertex weights. The second specifies edge "
    "\n   weights. You can use asterisk (*) as a wildcard for any intensity value."
    "\n   Weights can be specified as an integer value, or as a randomly generated"
    "\n   value. For the latter, use the notation 'U n1 n2' for the uniform dist."
    "\n   and 'N n1 n2' for the normal dist. with mean n1 and s.d. n2. The order in"
    "\n   which the rules are specified matters, as the later rules replace the"
    "\n   earlier ones.";

  cout << usage << endl;
  return -1;
}


int main(int argc, char *argv[])
{
  // Check arguments
  if(argc < 4) return usage();

  // Variables to hold command line arguments
  ImageGraphCutParameters p;
  p.fnInput = argv[argc-3];
  p.fnOutput = argv[argc-2];
  p.nParts = atoi(argv[argc-1]);
  p.xWeights.set_size(p.nParts);
  p.xWeights.fill(1.0 / p.nParts);

  // Read the options
  for(unsigned int iArg=1;iArg<argc-3;iArg++)
  {
    if(!strcmp(argv[iArg],"-w"))
    {
      unsigned int iPart = atoi(argv[++iArg]);
      double xWeightPart = atof(argv[++iArg]);
      if(iPart < p.nParts)
      {
        p.xWeights[iPart] = xWeightPart;
      }
      else
      {
        cerr << "Incorrect index in -w parameter" << endl;
        return usage();
      }
    }
    else if(!strcmp(argv[iArg],"-p"))
    {
      p.iPlaneDim = atoi(argv[++iArg]);
      p.iPlaneSlice = atoi(argv[++iArg]);
      p.iPlaneStrength = atoi(argv[++iArg]);
    }
    else if(!strcmp(argv[iArg],"-o"))
    {
      p.flagOptimize = true;
    }
    else if(!strcmp(argv[iArg],"-seed"))
    {
      p.use_random_seed = true;
      p.random_seed = atoi(argv[++iArg]);
      // srand(atoi(argv[++iArg]));
    }
    else if(!strcmp(argv[iArg], "-u"))
    {
      p.tolerance = atof(argv[++iArg]);
    }
    else if(!strcmp(argv[iArg], "-n"))
    {
      p.nMetisIter = atoi(argv[++iArg]);
    }
    else if(!strcmp(argv[iArg], "-c"))
    {
      p.max_comp = atoi(argv[++iArg]);
      p.min_comp_frac = atof(argv[++iArg]);
    }
    else
    {
      cerr << "unknown option!" << endl;
      return usage();
    }
  }

  return image_graph_cut(p);
}
