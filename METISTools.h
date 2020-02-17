#ifndef __METISTools_h_
#define __METISTools_h_

#include "ImageToGraphFilter.h"
#include <itkSingleValuedCostFunction.h>
#include <itkOnePlusOneEvolutionaryOptimizer.h>
#include <itkNormalVariateGenerator.h>
#include <vnl/vnl_cost_function.h>
#include <vnl/algo/vnl_powell.h>
#include <metis.h>

using namespace itk;

typedef int idxtype;

/** Function to run METIS using ImageToGraphFilter */
template< class TImage >
int RunMETISPartition(
  ImageToGraphFilter<TImage> *fltGraph,
  int nParts,
  float *xPartWeights,
  int *outPartition,
  float tolerance = 1.001,
  int nTries = 1,
  bool useRecursiveAlgorithm = true)
{
  // Variables used to call METIS
  int nVertices = fltGraph->GetNumberOfVertices();
  int nConstraints = 1;
  int wgtflag = 3;
  int numflag = 0;
  int edgecut = 0;
  float ubvec = tolerance;

  int options[METIS_NOPTIONS];
  METIS_SetDefaultOptions(options); 
  options[METIS_OPTION_CONTIG] = 1;
  options[METIS_OPTION_MINCONN] = 1;
  options[METIS_OPTION_CCORDER] = 1;
  options[METIS_OPTION_NCUTS] = nTries;

  if( useRecursiveAlgorithm )
    {
    METIS_PartGraphRecursive(
      &nVertices,
      &nConstraints,
      fltGraph->GetAdjacencyIndex(),
      fltGraph->GetAdjacency(),
      fltGraph->GetVertexWeights(),
      NULL,                                 // vsize ?
      fltGraph->GetEdgeWeights(),
      &nParts,
      xPartWeights,                         // tpweights
      &ubvec,                               // ubvec
      options,                              // options
      &edgecut,
      outPartition);
    }
  else
    {
    printf("Using K-way algorithm\n");
    METIS_PartGraphKway(
      &nVertices,
      &nConstraints,
      fltGraph->GetAdjacencyIndex(),
      fltGraph->GetAdjacency(),
      fltGraph->GetVertexWeights(),
      NULL,                                 // vsize ?
      fltGraph->GetEdgeWeights(),
      &nParts,
      xPartWeights,                         // tpweights
      &ubvec,                                 // ubvec
      options,                              // options
      &edgecut,
      outPartition);
    }  

  return edgecut;
}

/*
 * METIS PARTITION OPTIMIZATION
 *
 * This experimental code is used to optimize the METIS result over the
 * relative weights of the partitions. This optimization calls METIS in
 * the inner loop and therefore can be a little slow.
 */

class MetisPartitionProblem : public SingleValuedCostFunction
{
public:
  typedef MetisPartitionProblem Self;
  typedef SingleValuedCostFunction Superclass;
  typedef SmartPointer<Self> Pointer;
  typedef SmartPointer<const Self> ConstPointer;
  
  itkTypeMacro(MetisPartitionProblem, SingleValuedCostFunction);

  itkNewMacro(Self);
  
  typedef itk::Image<short,3> ImageType; 
  typedef ImageToGraphFilter<ImageType> GraphFilter;

  typedef Superclass::MeasureType MeasureType;
  typedef Superclass::ParametersType ParametersType;
  typedef Superclass::DerivativeType DerivativeType;

  /** Set the problem parameters */
  void SetProblem(GraphFilter *fltGraph, unsigned int nParts);

  /** Return the number of parameters */
  unsigned int GetNumberOfParameters() const override
    { return m_NumberOfParameters; }
  
  /** Virtual method from parent class */
  MeasureType GetValue(const ParametersType &x) const override;

  /** Not used, since there are no derivatives to evaluate */
  void GetDerivative(const ParametersType &, DerivativeType &) const override {}

  /** Get the result of the last partition */
  idxtype *GetLastPartition() { return m_Partition; }
  
private:
  /** The stored graph information */
  GraphFilter *m_Graph;

  /** The partition array */
  idxtype *m_Partition;

  /** Problem size */
  unsigned int m_NumberOfParameters;
};


#endif // __METISTools_h_
