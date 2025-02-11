#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "ImageGraphCut.h"

namespace py = pybind11;

void py_image_graph_cut(std::string fn_input,
                   std::string fn_output,
                   int n_parts,
                   const std::vector<double> weights,
                   bool optimize_weights,
                   float tolerance,
                   int n_iter,
                   int max_comp,
                   double min_comp_frac)
{
  ImageGraphCutParameters pd;
  pd.fnInput = fn_input;
  pd.fnOutput = fn_output;
  pd.nParts = n_parts;
  if(weights.size() == 0)
  {
    pd.xWeights.set_size(pd.nParts);
    pd.xWeights.fill(1.0 / pd.nParts);
  }
  else if(weights.size() == pd.nParts)
  {
    pd.xWeights.set_size(weights.size());
    for(int i = 0; i < pd.xWeights.size(); i++)
      pd.xWeights[i] = weights[i];
  }
  else
  {
    throw std::string("Incorrect number of weights");
  }
  pd.flagOptimize = optimize_weights;
  pd.tolerance = tolerance;
  pd.nMetisIter = n_iter;
  pd.max_comp = max_comp;
  pd.min_comp_frac = min_comp_frac;

  image_graph_cut(pd);
}


PYBIND11_MODULE(picsl_image_graph_cut, m) {
  // Default parameters
  ImageGraphCutParameters pd;
  m.doc() = "PICSL Image Graph Cut module";
  m.def("image_graph_cut", &py_image_graph_cut,
        py::arg("fn_input"),
        py::arg("fn_output"),
        py::arg("n_parts"),
        py::arg("weights") = std::vector<double>(),
        py::arg("optimize_weights") = pd.flagOptimize,
        py::arg("tolerance") = pd.tolerance,
        py::arg("n_metis_iter") = pd.nMetisIter,
        py::arg("max_comp") = pd.max_comp,
        py::arg("min_comp_frac") = pd.min_comp_frac,
        R"pbdoc(
            Cut a binary 3D image into a fixed number of partitions.

            Parameters:
                fn_input (str): Input image filename
                fn_output (str): Output image filename
                n_parts (int): Number of parts to partition the image into
                weights (List[float], optional): Weights of the individual partitions
                optimize_weights (bool, optional): Optimize the weigths, defaults to false
                tolerance (float, optional):
                    Load imbalance tolerance (ubvec in METIS.
                    Must be >= 1. Larger values means more flexibility for non-equal partitions
                n_metis_iter (int, optional): Number of iterations of internal METIS optimization
                max_comp (int, optional):
                    Keep only the N largest connected components in the input image
                min_comp_frac (float, optional):
                    Remove connected components in the input image that are larger than
                    this fraction of total volume.
        )pbdoc");
}

