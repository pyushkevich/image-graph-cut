#ifndef __ImageGraphCut_h_
#define __ImageGraphCut_h_

#include <string>
#include <vnl/vnl_vector.h>

struct ImageGraphCutParameters
{
  // Variables to hold command line arguments
  std::string fnInput, fnOutput;
  int nParts;
  vnl_vector<float> xWeights;
  int iPlaneDim = -1, iPlaneSlice = -1, iPlaneStrength = 10;
  bool flagOptimize = false;
  float tolerance = 1.001;
  int nMetisIter = 1;
  int max_comp = 1;
  double min_comp_frac = 0.0;
  bool use_random_seed = false;
  int random_seed = 0;
};

int image_graph_cut(const ImageGraphCutParameters &p);

#endif
