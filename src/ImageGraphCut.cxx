#include <iostream>
#include "ImageGraphCut.h"
#include "METISTools.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkConnectedComponentImageFilter.h"
#include "itkRelabelComponentImageFilter.h"
#include "itkImageRegionConstIterator.h"
#include "itkBinaryThresholdImageFilter.h"

using namespace std;
using namespace itk;

/* ***************************************************************************
 * GLOBAL TYPE DEFINITIONS
 * *************************************************************************** */
typedef itk::Image< short, 3 > ImageType;
typedef ImageToGraphFilter< ImageType > GraphFilter;
typedef GraphFilter::WeightFunctorType BaseWeightFunctor;
typedef vnl_vector<float> Vec;

/* ***************************************************************************
 * WEIGHT TABLE CODE
 * *************************************************************************** */
class MyWeightFunctor : public BaseWeightFunctor
{
public:

  /** Read table from file */
  bool ReadTable(const char *file) { return false; };

  /** Check inclusion */
  virtual bool IsPixelAVertex(short i1)
  {
    return i1 != 0;
  }
  
  /** Compute edge weight */
  virtual int GetEdgeWeight(short i1, short i2)
  {
    return 1;
  }

  /** Compute vertex weight */
  virtual int GetVertexWeight(short i1)
  {
    return 1;
  }
};

/* ***************************************************************************
 * GRAPH VERIFICATION
 * *************************************************************************** */
template<class T, class S>
void VerifyGraph(int n, T *ai, T *a, S *wv, S *we)
{
  for(T i=0; i < n; i++)
  {
    T k = ai[i+1] - ai[i];
    for(T j=0;j < k;j++)
    {
      // Neighbor of i
      T m = a[ai[i] + j];
      S w = we[ai[i] + j];

             // Check the match
      T l = ai[m+1] - ai[m];
      bool match = false;
      for(T p=0;p < l;p++)
        if(a[ai[m]+p] == i && we[ai[m]+p] == w)
        { match = true; break; }

      if(!match)
      {
        cout << "Mismatch at node " << i << " edge to " << m << " weight " << w << endl;
      }
      if(w <= 0 || w > 1000 || wv[i] <= 0 || wv[i] > 1000)
      {
        cout << " bad weight" << i << endl;
      }
    }
  }
}

Vec OptimizeMETISPartition(GraphFilter *fltGraph, const Vec &xWeights)
{
  // Create a METIS problem based on the graph and weights
  MetisPartitionProblem::Pointer mp = MetisPartitionProblem::New();
  mp->SetProblem(fltGraph, xWeights.size() - 1);

  // Get the starting solution
  MetisPartitionProblem::ParametersType x( xWeights.size() - 1 );
  for(unsigned int j = 0; j < x.size(); j++)
    x[j] = xWeights[j];
  
  typedef OnePlusOneEvolutionaryOptimizer Optimizer;
  Optimizer::Pointer opt = Optimizer::New();

  typedef itk::Statistics::NormalVariateGenerator Generator;
  Generator::Pointer generator = Generator::New();

  opt->SetCostFunction(mp);
  opt->SetInitialPosition(x);
  opt->SetInitialRadius(0.005);
  opt->SetMaximumIteration(100);
  opt->SetNormalVariateGenerator(generator);
  opt->StartOptimization();

  x = opt->GetCurrentPosition();
  Vec xResult(xWeights.size(), 0.0);
  xResult[xWeights.size() - 1] = 1.0;
  for(unsigned int i = 0; i < xWeights.size() - 1; i++)
  {
    xResult[i] = x[i];
    xResult[xWeights.size() - 1] -= x[i];
  }
  
  return xResult;
}


int image_graph_cut(const ImageGraphCutParameters &p)
{
  // Set random seed
  if(p.use_random_seed)
    srand(p.random_seed);

  // Write partition information
  cout << "will generate " << p.nParts << " partitions" << endl;
  float xWeightSum = 0.0f;
  for(unsigned int iPart = 0;iPart < p.nParts;iPart++)
  {
    cout << "   part " << iPart << "\t weight " << p.xWeights[iPart] << endl;
    xWeightSum += p.xWeights[iPart];
  }
  cout << "   total of weights : " << xWeightSum << endl;
  if(p.iPlaneDim >= 0)
  {
    cout << "will insert cut plane at slice " << p.iPlaneSlice
         << " in dimension " << p.iPlaneDim
         << " with strength " << p.iPlaneStrength << endl;
  }
  cout << endl;

  // Read the input image image
  cout << "reading input image" << endl;

  typedef ImageFileReader<ImageType> ReaderType;
  ReaderType::Pointer fltReader = ReaderType::New();
  fltReader->SetFileName(p.fnInput.c_str());
  fltReader->Update();
  ImageType::Pointer img = fltReader->GetOutput();

         // Extract connected components
  typedef itk::ConnectedComponentImageFilter<ImageType, ImageType> ConnFilter;
  ConnFilter::Pointer conn_filter = ConnFilter::New();
  conn_filter->SetInput(img);
  conn_filter->Update();
  conn_filter->SetFullyConnected(false);

  typedef itk::RelabelComponentImageFilter<ImageType, ImageType> RelabelFilter;
  RelabelFilter::Pointer relabel_filter = RelabelFilter::New();
  relabel_filter->SetInput(conn_filter->GetOutput());
  relabel_filter->Update();
  ImageType::Pointer comp_map_image = relabel_filter->GetOutput();

         // Get a list of connected components and their size
  unsigned int hist_size = p.max_comp + 1;
  std::vector<unsigned int> comp_histogram(hist_size, 0);
  unsigned int n_total = 0;
  for(itk::ImageRegionConstIterator<ImageType> it(comp_map_image, comp_map_image->GetBufferedRegion());
       !it.IsAtEnd(); ++it)
  {
    short val = it.Value();
    if(val > 0 && val < hist_size)
    {
      comp_histogram[val]++;
      n_total++;
    }
  }

         // Compute the total number of pixels and proportion of each component
  std::map<short, unsigned int> comp_parts;
  for(unsigned int i = 1; i < hist_size; i++)
  {
    double frac = comp_histogram[i] * 1.0 / n_total;
    if(frac >= p.min_comp_frac)
    {
      int n_parts = std::max(1, (int)(0.5 + p.nParts * frac));
      comp_parts[i] = n_parts;
      std::cout << "Keeping component " << i << " fraction " << frac << " parts " << n_parts << endl;
    }
  }

  cout << "   image has dimensions " << img->GetBufferedRegion().GetSize()
       << ", nPixels = " << img->GetBufferedRegion().GetNumberOfPixels()
       << ", nComp = " << comp_parts.size() << endl;

  // Create output image
  ImageType::Pointer imgOut = ImageType::New();
  imgOut->SetRegions(img->GetBufferedRegion());
  imgOut->CopyInformation(img);
  imgOut->Allocate();
  imgOut->FillBuffer(0);

  // Repeat for each component
  unsigned int part_idx = 1;
  for(auto comp : comp_parts)
  {
    typedef itk::BinaryThresholdImageFilter<ImageType, ImageType> ThreshFilter;
    ThreshFilter::Pointer fltThresh = ThreshFilter::New();
    fltThresh->SetLowerThreshold(comp.first);
    fltThresh->SetUpperThreshold(comp.first);
    fltThresh->SetInsideValue(1);
    fltThresh->SetOutsideValue(0);
    fltThresh->SetInput(comp_map_image);
    fltThresh->Update();

           // This is the image that we will partition now
    ImageType::Pointer comp_image = fltThresh->GetOutput();

           // Use the relative weights only if number of components matches
    auto compWeights = (p.xWeights.size() == comp.second)
                         ? p.xWeights
                         : vnl_vector<float>(comp.second, 1.0/comp.second);

    cout << "   Breaking component " << comp.first << " into " << comp.second << " parts. " << endl;
    cout << "      Initial weights: " << compWeights << endl;
    cout << "      Starting part is: " << part_idx << endl;

    unsigned int max_part=1;
    if(comp.second > 1)
    {
      // Create the weight functor
      MyWeightFunctor fnWeight;

      // Create the graph filter
      typedef ImageToGraphFilter<ImageType,idxtype> GraphFilter;
      GraphFilter::Pointer fltGraph = GraphFilter::New();
      fltGraph->SetInput(comp_image);
      fltGraph->SetWeightFunctor(&fnWeight);
      fltGraph->Update();

      // If asked to optimize, compute the best set of weights
      if(p.flagOptimize)
      {
        // Run the experimental optimization
        compWeights = OptimizeMETISPartition(fltGraph, compWeights);
        cout << "      Optimized weights: " << compWeights << endl;
      }

      // Run METIS once, using the specified weights
      int *iPartition = new int[fltGraph->GetNumberOfVertices()];
      int xCut = RunMETISPartition<ImageType>(
        fltGraph, compWeights.size(), compWeights.data_block(), iPartition,
        p.tolerance, p.nMetisIter, false);
      cout << "      Cut value: " << xCut << endl;

      // Apply partition to output image
      for(unsigned int iVertex = 0; iVertex < fltGraph->GetNumberOfVertices(); iVertex++)
      {
        GraphFilter::IndexType idx = fltGraph->GetVertexImageIndex(iVertex);
        imgOut->SetPixel(idx, iPartition[iVertex] + part_idx);
        max_part = std::max(max_part, (unsigned int ) iPartition[iVertex]);
      }

      // Delete the partition
      delete [] iPartition;
    }
    else
    {
      typedef itk::ImageRegionIterator<ImageType> IterType;
      IterType it_src(comp_image, comp_image->GetBufferedRegion());
      IterType it_dst(imgOut, imgOut->GetBufferedRegion());
      for(; !it_src.IsAtEnd(); ++it_src, ++it_dst)
        if(it_src.Value() > 0)
          it_dst.Set(part_idx);
    }

    // Update the starting part
    part_idx += max_part + 1;
  }

  // Write the image
  cout << "writing output image" << endl;

  typedef ImageFileWriter<ImageType> WriterType;
  WriterType::Pointer fltWriter = WriterType::New();
  fltWriter->SetInput(imgOut);
  fltWriter->SetFileName(p.fnOutput.c_str());
  fltWriter->Update();

  // Done!
  return 0;
}

