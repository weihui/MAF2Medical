/*=========================================================================
Program:   Multimod Application Framework
Module:    $RCSfile: vtkMEDPoissonSurfaceReconstruction.h,v $
Language:  C++
Date:      $Date: 2010-06-16 08:16:02 $
Version:   $Revision: 1.1.2.2 $
Authors:   Fuli Wu
==========================================================================
Copyright (c) 2001/2005 
CINECA - Interuniversity Consortium (www.cineca.it)
=========================================================================*/

#ifndef __vtkMEDPoissonSurfaceReconstruction_h
#define __vtkMEDPoissonSurfaceReconstruction_h

//----------------------------------------------------------------------------
// Include:
//----------------------------------------------------------------------------
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkDataSetToPolyDataFilter.h"

#include <vector>
#include <hash_map>
#include <algorithm>

//----------------------------------------------------------------------------
// forward declarations :
//----------------------------------------------------------------------------

using stdext::hash_map;

/**
class name: vtkMEDPoissonSurfaceReconstruction

This class implement Poisson Surface Reconstruction method.
A paper can be viewed here: research.microsoft.com/en-us/um/people/hoppe/poissonrecon.pdf
*/
class VTK_GRAPHICS_EXPORT vtkMEDPoissonSurfaceReconstruction : public vtkDataSetToPolyDataFilter
{
public:
  /** create instance of the object */
  static vtkMEDPoissonSurfaceReconstruction *New();
  /** RTTI macro */
  vtkTypeRevisionMacro(vtkMEDPoissonSurfaceReconstruction,vtkDataSetToPolyDataFilter);
  /** print object information */
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This error function allows our ported code to report error messages neatly.
  // This is not for external use. 
  void Error(const char *message);

protected:
  /** constructor */
  vtkMEDPoissonSurfaceReconstruction();
  /** destructor */
  ~vtkMEDPoissonSurfaceReconstruction();

  // Description:
  // the main function that does the work
  void Execute();

  /** computation of extents and update values*/
  void ComputeInputUpdateExtents(vtkDataObject *output);
  /** only check if input is not null */
  void ExecuteInformation(); 
  
private:
  /** copy constructor not implemented */
  vtkMEDPoissonSurfaceReconstruction(const vtkMEDPoissonSurfaceReconstruction&);
  /** operator= non implemented */
  void operator=(const vtkMEDPoissonSurfaceReconstruction&);
};


/*=========================================================================
Copyright (c) 2006, Michael Kazhdan and Matthew Bolitho
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer. Redistributions in binary form must reproduce
the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution. 

Neither the name of the Johns Hopkins University nor the names of its contributors
may be used to endorse or promote products derived from this software without specific
prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
=========================================================================*/


#define PI 3.1415926535897932384
#define SQRT_3 1.7320508075688772935
#define DIMENSION 3

#define ITERATION_POWER   ((double)(1.0/3))
#define MEMORY_ALLOCATOR_BLOCK_SIZE 1<<12

#define READ_SIZE 1024

#define PAD_SIZE (Real(1.0))


#define SCALE 1.25

double ArcTan2(const double& y,const double& x);
double Angle(const double in[2]);
void Sqrt(const double in[2],double out[2]);
void Add(const double in1[2],const double in2[2],double out[2]);
void Subtract(const double in1[2],const double in2[2],double out[2]);
void Multiply(const double in1[2],const double in2[2],double out[2]);
void Divide(const double in1[2],const double in2[2],double out[2]);

int Factor(double a1,double a0,double roots[1][2],const double& EPS);
int Factor(double a2,double a1,double a0,double roots[2][2],const double& EPS);
int Factor(double a3,double a2,double a1,double a0,double roots[3][2],const double& EPS);
int Factor(double a4,double a3,double a2,double a1,double a0,double roots[4][2],const double& EPS);

int Solve(const double* eqns,const double* values,double* solutions,const int& dim);

void PSR_main();

/*=========================================================================
Here, for header file definition
=========================================================================*/


/*=========================================================================
Allocate.h
=========================================================================*/

/** 
      class name: AllocatorState
     This templated class assists in memory allocation and is well suited for instances
      when it is known that the sequence of memory allocations is performed in a stack-based
      manner, so that memory allocated last is released first. It also preallocates memory
      in chunks so that multiple requests for small chunks of memory do not require separate
      system calls to the memory manager.
      The allocator is templated off of the class of objects that we would like it to allocate,
      ensuring that appropriate constructors and destructors are called as necessary.
  */

class AllocatorState{
public:
	int index,remains;
};

template<class T>
/**
class name: Allocator
memory handler
*/
class Allocator{
	int blockSize;
	int index,remains;
	std::vector<T*> memory;
public:
  /** constructor */
	Allocator(void){
		blockSize=index=remains=0;
	}
  /** destructor */
	~Allocator(void){
		reset();
	}

	/** This method is the allocators destructor. It frees up any of the memory that
	  * it has allocated. */
	void reset(void){
		for(size_t i=0;i<memory.size();i++){delete[] memory[i];}
		memory.clear();
		blockSize=index=remains=0;
	}
	/** This method returns the memory state of the allocator. */
	AllocatorState getState(void) const{
		AllocatorState s;
		s.index=index;
		s.remains=remains;
		return s;
	}


	/** This method rolls back the allocator so that it makes all of the memory previously
	  * allocated available for re-allocation. Note that it does it not call the constructor
	  * again, so after this method has been called, assumptions about the state of the values
	  * in memory are no longer valid. */
	void rollBack(void){
		if(memory.size()){
			for(size_t i=0;i<memory.size();i++){
				for(int j=0;j<blockSize;j++){
					memory[i][j].~T();
					new(&memory[i][j]) T();
				}
			}
			index=0;
			remains=blockSize;
		}
	}
	/** This method rolls back the allocator to the previous memory state and makes all of the memory previously
	  * allocated available for re-allocation. Note that it does it not call the constructor
	  * again, so after this method has been called, assumptions about the state of the values
	  * in memory are no longer valid. */
	void rollBack(const AllocatorState& state){
		if(state.index<index || (state.index==index && state.remains<remains)){
			if(state.index<index){
				for(int j=state.remains;j<blockSize;j++){
					memory[state.index][j].~T();
					new(&memory[state.index][j]) T();
				}
				for(int i=state.index+1;i<index-1;i++){
					for(int j=0;j<blockSize;j++){
						memory[i][j].~T();
						new(&memory[i][j]) T();
					}
				}
				for(int j=0;j<remains;j++){
					memory[index][j].~T();
					new(&memory[index][j]) T();
				}
				index=state.index;
				remains=state.remains;
			}
			else{
				for(int j=0;j<state.remains;j<remains){
					memory[index][j].~T();
					new(&memory[index][j]) T();
				}
				remains=state.remains;
			}
		}
	}

	/** This method initiallizes the constructor and the blockSize variable specifies the
	  * the number of objects that should be pre-allocated at a time. */
	void set(const int& blockSize){
		reset();
		this->blockSize=blockSize;
		index=-1;
		remains=0;
	}

	/** This method returns a pointer to an array of elements objects. If there is left over pre-allocated
	  * memory, this method simply returns a pointer to the next free piece of memory, otherwise it pre-allocates
	  * more memory. Note that if the number of objects requested is larger than the value blockSize with which
	  * the allocator was initialized, the request for memory will fail.
	  */
	T* newElements(const int& elements=1){
		T* mem;
		if(!elements){return NULL;}
		if(elements>blockSize){
			fprintf(stderr,"Allocator Error, elements bigger than block-size: %d>%d\n",elements,blockSize);
			return NULL;
		}
		if(remains<elements){
			if(index==memory.size()-1){
				mem=new T[blockSize];
				if(!mem){fprintf(stderr,"Failed to allocate memory\n");exit(0);}
				memory.push_back(mem);
			}
			index++;
			remains=blockSize;
		}
		mem=&(memory[index][blockSize-remains]);
		remains-=elements;
		return mem;
	}
};

/*=========================================================================
BindaryNode.h
=========================================================================*/
/**
class name: BinaryNode
template class which represent a binary node of a binary tree, infact  the topology of the octree (used inside poisson surface reconstruction) 
defines a set of binary trees.
*/
template<class Real>
class BinaryNode{
public:
  /**  return double depth, not used in the filter */
	static inline int CenterCount(int depth){return 1<<depth;}
  /** return  maxdepth after increment, double and decrement, used for calculate number of base functions of polyomial*/
	static inline int CumulativeCenterCount(int maxDepth){return (1<<(maxDepth+1))-1;}
  /** retrieve index giving tree depth and an offset*/
	static inline int Index(int depth, int offSet){return (1<<depth)+offSet-1;}
  /** retrieve index  of the node in the corner*/
	static inline int CornerIndex(int maxDepth,int depth,int offSet,int forwardCorner)
	  {return (offSet+forwardCorner)<<(maxDepth-depth);}
  /** retrieve position  of the node in the corner*/
	static inline Real CornerIndexPosition(int index,int maxDepth)
	  {return Real(index)/(1<<maxDepth);}
  /**retrieve the width of the node */
	static inline Real Width(int depth)
	  {return Real(1.0/(1<<depth));}
  /**retrieve the width and the center  of the node */
	static inline void CenterAndWidth(int depth,int offset,Real& center,Real& width)
	  {
	    width=Real(1.0/(1<<depth));
	    center=Real((0.5+offset)*width);
	  }
  /**retrieve the width and the center  of the node */
	static inline void CenterAndWidth(int idx,Real& center,Real& width)
	  {
	    int depth,offset;
	    DepthAndOffset(idx,depth,offset);
	    CenterAndWidth(depth,offset,center,width);
	  }
  /**retrieve the depth and the offset  of the node */
	static inline void DepthAndOffset(int idx, int& depth,int& offset)
	  {
	    int i=idx+1;
	    depth=-1;
	    while(i){
	      i>>=1;
	      depth++;
	    }
	    offset=(idx+1)-(1<<depth);
	  }
};

/*=========================================================================
Polynomial.h
=========================================================================*/
/**
class name: Polynomial
Template class that represents a polynomial with a specific degree.
*/
template<int Degree>
class Polynomial{
public:
	double coefficients[Degree+1];

  /** constructor */
	Polynomial(void);
  /** copy constructor */
	template<int Degree2>
	Polynomial(const Polynomial<Degree2>& P);
  /** overload operator () which retrieves  the sum of the product of the coefficients*/
	double operator()(const double& t) const;
  /** calculate integral */
	double integral(const double& tMin,const double& tMax) const;

  /** operator== overload , checking coefficients */
	int operator == (const Polynomial& p) const;
  /** operator!= overload, checking coefficients */
	int operator != (const Polynomial& p) const;
  /** check if all coefficients are zero*/
	int isZero(void) const;
  /** set  all coefficients as zero*/
	void setZero(void);

  /** overload operator, according to the operation over coefficients */
	template<int Degree2>
	Polynomial& operator  = (const Polynomial<Degree2> &p);
  /** overload operator, according to the operation over coefficients */
	Polynomial& operator += (const Polynomial& p);
  /** overload operator, according to the operation over coefficients */
	Polynomial& operator -= (const Polynomial& p);
  /** overload operator, according to the operation over coefficients */
	Polynomial  operator -  (void) const;
  /** overload operator, according to the operation over coefficients */
	Polynomial  operator +  (const Polynomial& p) const;
  /** overload operator, according to the operation over coefficients */
	Polynomial  operator -  (const Polynomial& p) const;
  /** overload operator, according to the operation over coefficients */
	template<int Degree2>
	Polynomial<Degree+Degree2>  operator *  (const Polynomial<Degree2>& p) const;

  /** overload operator, according to the operation over coefficients */
	Polynomial& operator += (const double& s);
  /** overload operator, according to the operation over coefficients */
	Polynomial& operator -= (const double& s);
  /** overload operator, according to the operation over coefficients */
	Polynomial& operator *= (const double& s);
  /** overload operator, according to the operation over coefficients */
	Polynomial& operator /= (const double& s);
  /** overload operator, according to the operation over coefficients */
	Polynomial  operator +  (const double& s) const;
  /** overload operator, according to the operation over coefficients */
	Polynomial  operator -  (const double& s) const;
  /** overload operator, according to the operation over coefficients */
	Polynomial  operator *  (const double& s) const;
  /** overload operator, according to the operation over coefficients */
	Polynomial  operator /  (const double& s) const;

  /** overload operator, according to the operation over coefficients */
	Polynomial scale(const double& s) const;
  /** overload operator, according to the operation over coefficients */
	Polynomial shift(const double& t) const;

  /** calculate derivative */
	Polynomial<Degree-1> derivative(void) const;
  /** calculate integral */
	Polynomial<Degree+1> integral(void) const;

  /** print representation of polynomial */
	void printnl(void) const;

  /** overload operator, according to the operation over coefficients */
	Polynomial& addScaled(const Polynomial& p,const double& scale);

  /** overload operator, according to the operation over coefficients */
	static void Negate(const Polynomial& in,Polynomial& out);
  /** overload operator, according to the operation over coefficients */
	static void Subtract(const Polynomial& p1,const Polynomial& p2,Polynomial& q);
  /** overload operator, according to the operation over coefficients */
	static void Scale(const Polynomial& p,const double& w,Polynomial& q);
  /** overload operator, according to the operation over coefficients */
	static void AddScaled(const Polynomial& p1,const double& w1,const Polynomial& p2,const double& w2,Polynomial& q);
  /** overload operator, according to the operation over coefficients */
	static void AddScaled(const Polynomial& p1,const Polynomial& p2,const double& w2,Polynomial& q);
  /** overload operator, according to the operation over coefficients */
	static void AddScaled(const Polynomial& p1,const double& w1,const Polynomial& p2,Polynomial& q);

  /** calculate roots */
	void getSolutions(const double& c,std::vector<double>& roots,const double& EPS) const;
};

/*=========================================================================
PPolynomial.h
=========================================================================*/
/**
class name: StartingPolynomial
//CONTINUE FROM HERE
*/
template<int Degree>
class StartingPolynomial{
public:
	Polynomial<Degree> p;
	double start;

	template<int Degree2>
	StartingPolynomial<Degree+Degree2>  operator * (const StartingPolynomial<Degree2>& p) const;
	StartingPolynomial scale(const double& s) const;
	StartingPolynomial shift(const double& t) const;
	int operator < (const StartingPolynomial& sp) const;
	static int Compare(const void* v1,const void* v2);
};

template<int Degree>
class PPolynomial{
public:
	size_t polyCount;
	StartingPolynomial<Degree>* polys;

	PPolynomial(void);
	PPolynomial(const PPolynomial<Degree>& p);
	~PPolynomial(void);

	PPolynomial& operator = (const PPolynomial& p);

	int size(void) const;

	void set(const size_t& size);
	// Note: this method will sort the elements in sps
	void set(StartingPolynomial<Degree>* sps,const int& count);
	void reset(const size_t& newSize);


	double operator()(const double& t) const;
	double integral(const double& tMin,const double& tMax) const;
	double Integral(void) const;

	template<int Degree2>
	PPolynomial<Degree>& operator = (const PPolynomial<Degree2>& p);

	PPolynomial  operator + (const PPolynomial& p) const;
	PPolynomial  operator - (const PPolynomial& p) const;

	template<int Degree2>
	PPolynomial<Degree+Degree2> operator * (const Polynomial<Degree2>& p) const;

	template<int Degree2>
	PPolynomial<Degree+Degree2> operator * (const PPolynomial<Degree2>& p) const;


	PPolynomial& operator += (const double& s);
	PPolynomial& operator -= (const double& s);
	PPolynomial& operator *= (const double& s);
	PPolynomial& operator /= (const double& s);
	PPolynomial  operator +  (const double& s) const;
	PPolynomial  operator -  (const double& s) const;
	PPolynomial  operator *  (const double& s) const;
	PPolynomial  operator /  (const double& s) const;

	PPolynomial& addScaled(const PPolynomial& poly,const double& scale);

	PPolynomial scale(const double& s) const;
	PPolynomial shift(const double& t) const;

	PPolynomial<Degree-1> derivative(void) const;
	PPolynomial<Degree+1> integral(void) const;

	void getSolutions(const double& c,std::vector<double>& roots,const double& EPS,const double& min=-DBL_MAX,const double& max=DBL_MAX) const;

	void printnl(void) const;

	PPolynomial<Degree+1> MovingAverage(const double& radius);

	static PPolynomial ConstantFunction(const double& width=0.5);
	static PPolynomial GaussianApproximation(const double& width=0.5);
//	void write(FILE* fp,const int& samples,const double& min,const double& max) const;
};


/*=========================================================================
FunctionData.h
=========================================================================*/
template<int Degree,class Real>
class FunctionData{
	int useDotRatios;
	int normalize;
public:
	const static int     DOT_FLAG;
	const static int   D_DOT_FLAG;
	const static int  D2_DOT_FLAG;
	const static int   VALUE_FLAG;
	const static int D_VALUE_FLAG;

	int depth,res,res2;
	Real *dotTable,*dDotTable,*d2DotTable;
	Real *valueTables,*dValueTables;
	PPolynomial<Degree> baseFunction;
	PPolynomial<Degree-1> dBaseFunction;
	PPolynomial<Degree+1>* baseFunctions;

	FunctionData(void);
	~FunctionData(void);

	virtual void   setDotTables(const int& flags);
	virtual void clearDotTables(const int& flags);

	virtual void   setValueTables(const int& flags,const double& smooth=0);
	virtual void   setValueTables(const int& flags,const double& valueSmooth,const double& normalSmooth);
	virtual void clearValueTables(void);

	void set(const int& maxDepth,const PPolynomial<Degree>& F,const int& normalize,const int& useDotRatios=1);

	Real   dotProduct(const double& center1,const double& width1,const double& center2,const double& width2) const;
	Real  dDotProduct(const double& center1,const double& width1,const double& center2,const double& width2) const;
	Real d2DotProduct(const double& center1,const double& width1,const double& center2,const double& width2) const;

	static inline int SymmetricIndex(const int& i1,const int& i2);
	static inline int SymmetricIndex(const int& i1,const int& i2,int& index);
};

/*=========================================================================
Geometry.h
=========================================================================*/
template<class Real>
Real Random(void);

template<class Real>
struct Point3D{Real coords[3];};

template<class Real>
Point3D<Real> RandomBallPoint(void);

template<class Real>
Point3D<Real> RandomSpherePoint(void);

template<class Real>
double Length(const Point3D<Real>& p);

template<class Real>
double SquareLength(const Point3D<Real>& p);

template<class Real>
double Distance(const Point3D<Real>& p1,const Point3D<Real>& p2);

template<class Real>
double SquareDistance(const Point3D<Real>& p1,const Point3D<Real>& p2);

template <class Real>
void CrossProduct(const Point3D<Real>& p1,const Point3D<Real>& p2,Point3D<Real>& p);

class Edge{
public:
	double p[2][2];
	double Length(void) const{
		double d[2];
		d[0]=p[0][0]-p[1][0];
		d[1]=p[0][1]-p[1][1];

		return sqrt(d[0]*d[0]+d[1]*d[1]);
	}
};
class Triangle{
public:
	double p[3][3];
	double Area(void) const{
		double v1[3],v2[3],v[3];
		for(int d=0;d<3;d++){
			v1[d]=p[1][d]-p[0][d];
			v2[d]=p[2][d]-p[0][d];
		}
		v[0]= v1[1]*v2[2]-v1[2]*v2[1];
		v[1]=-v1[0]*v2[2]+v1[2]*v2[0];
		v[2]= v1[0]*v2[1]-v1[1]*v2[0];
		return sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])/2;
	}
	double AspectRatio(void) const{
		double d=0;
		int i,j;
		for(i=0;i<3;i++){
	  for(i=0;i<3;i++)
			for(j=0;j<3;j++){d+=(p[(i+1)%3][j]-p[i][j])*(p[(i+1)%3][j]-p[i][j]);}
		}
		return Area()/d;
	}
	
};
class CoredPointIndex{
public:
	int index;
	char inCore;

	int operator == (const CoredPointIndex& cpi) const {return (index==cpi.index) && (inCore==cpi.inCore);};
	int operator != (const CoredPointIndex& cpi) const {return (index!=cpi.index) || (inCore!=cpi.inCore);};
};
class EdgeIndex{
public:
	int idx[2];
};
class CoredEdgeIndex{
public:
	CoredPointIndex idx[2];
};
class TriangleIndex{
public:
	int idx[3];
};

class TriangulationEdge
{
public:
	TriangulationEdge(void);
	int pIndex[2];
	int tIndex[2];
};

class TriangulationTriangle
{
public:
	TriangulationTriangle(void);
	int eIndex[3];
};

template<class Real>
class Triangulation
{
public:

	std::vector<Point3D<Real> >		points;
	std::vector<TriangulationEdge>				edges;
	std::vector<TriangulationTriangle>			triangles;

	int factor(const int& tIndex,int& p1,int& p2,int& p3);
	double area(void);
	double area(const int& tIndex);
	double area(const int& p1,const int& p2,const int& p3);
	int flipMinimize(const int& eIndex);
	int addTriangle(const int& p1,const int& p2,const int& p3);

protected:
	hash_map<long long,int> edgeMap;
	static long long EdgeIndex(const int& p1,const int& p2);
	double area(const Triangle& t);
};


template<class Real>
void EdgeCollapse(const Real& edgeRatio,std::vector<TriangleIndex>& triangles,std::vector< Point3D<Real> >& positions,std::vector<Point3D<Real> >* normals);
template<class Real>
void TriangleCollapse(const Real& edgeRatio,std::vector<TriangleIndex>& triangles,std::vector<Point3D<Real> >& positions,std::vector<Point3D<Real> >* normals);

class CoredMeshData{
public:
	std::vector<Point3D<float> > inCorePoints;
	const static int IN_CORE_FLAG[3];
	virtual void resetIterator(void)=0;

	virtual int addOutOfCorePoint(const Point3D<float>& p)=0;
	virtual int addTriangle(const TriangleIndex& t,const int& icFlag=(IN_CORE_FLAG[0] | IN_CORE_FLAG[1] | IN_CORE_FLAG[2]))=0;

	virtual int nextOutOfCorePoint(Point3D<float>& p)=0;
	virtual int nextTriangle(TriangleIndex& t,int& inCoreFlag)=0;

	virtual int outOfCorePointCount(void)=0;
	virtual int triangleCount(void)=0;
};

class CoredVectorMeshData : public CoredMeshData{
	std::vector<Point3D<float> > oocPoints;
	std::vector<TriangleIndex> triangles;
	int oocPointIndex,triangleIndex;
public:
	CoredVectorMeshData::CoredVectorMeshData(void);

	void resetIterator(void);

	int addOutOfCorePoint(const Point3D<float>& p);
	int addTriangle(const TriangleIndex& t,const int& inCoreFlag=(CoredMeshData::IN_CORE_FLAG[0] | CoredMeshData::IN_CORE_FLAG[1] | CoredMeshData::IN_CORE_FLAG[2]));

	int nextOutOfCorePoint(Point3D<float>& p);
	int nextTriangle(TriangleIndex& t,int& inCoreFlag);

	int outOfCorePointCount(void);
	int triangleCount(void);
};

class CoredFileMeshData : public CoredMeshData{
	FILE *oocPointFile,*triangleFile;
	int oocPoints,triangles;
public:
	CoredFileMeshData(void);
	~CoredFileMeshData(void);

	void resetIterator(void);

	int addOutOfCorePoint(const Point3D<float>& p);
	int addTriangle(const TriangleIndex& t,const int& inCoreFlag=(CoredMeshData::IN_CORE_FLAG[0] | CoredMeshData::IN_CORE_FLAG[1] | CoredMeshData::IN_CORE_FLAG[2]));

	int nextOutOfCorePoint(Point3D<float>& p);
	int nextTriangle(TriangleIndex& t,int& inCoreFlag);

	int outOfCorePointCount(void);
	int triangleCount(void);
};


/*=========================================================================
MarchingCubes.h
=========================================================================*/
class Square{
public:
	const static int CORNERS=4,EDGES=4,NEIGHBORS=4;
	static int  CornerIndex			(const int& x,const int& y);
	static void FactorCornerIndex	(const int& idx,int& x,int& y);
	static int  EdgeIndex			(const int& orientation,const int& i);
	static void FactorEdgeIndex		(const int& idx,int& orientation,int& i);

	static int  ReflectCornerIndex	(const int& idx,const int& edgeIndex);
	static int  ReflectEdgeIndex	(const int& idx,const int& edgeIndex);

	static void EdgeCorners(const int& idx,int& c1,int &c2);
};

class Cube{
public:
	const static int CORNERS=8,EDGES=12,NEIGHBORS=6;

	static int  CornerIndex			(const int& x,const int& y,const int& z);
	static void FactorCornerIndex	(const int& idx,int& x,int& y,int& z);
	static int  EdgeIndex			(const int& orientation,const int& i,const int& j);
	static void FactorEdgeIndex		(const int& idx,int& orientation,int& i,int &j);
	static int  FaceIndex			(const int& dir,const int& offSet);
	static int  FaceIndex			(const int& x,const int& y,const int& z);
	static void FactorFaceIndex		(const int& idx,int& x,int &y,int& z);
	static void FactorFaceIndex		(const int& idx,int& dir,int& offSet);

	static int  AntipodalCornerIndex	(const int& idx);
	static int  FaceReflectCornerIndex	(const int& idx,const int& faceIndex);
	static int  FaceReflectEdgeIndex	(const int& idx,const int& faceIndex);
	static int	FaceReflectFaceIndex	(const int& idx,const int& faceIndex);
	static int	EdgeReflectCornerIndex	(const int& idx,const int& edgeIndex);
	static int	EdgeReflectEdgeIndex	(const int& edgeIndex);

	static int  FaceAdjacentToEdges	(const int& eIndex1,const int& eIndex2);
	static void FacesAdjacentToEdge	(const int& eIndex,int& f1Index,int& f2Index);

	static void EdgeCorners(const int& idx,int& c1,int &c2);
	static void FaceCorners(const int& idx,int& c1,int &c2,int& c3,int& c4);
};

class MarchingSquares{
	static double Interpolate(const double& v1,const double& v2);
	static void SetVertex(const int& e,const double values[Square::CORNERS],const double& iso);
public:
	const static int MAX_EDGES=2;
	static const int edgeMask[1<<Square::CORNERS];
	static const int edges[1<<Square::CORNERS][2*MAX_EDGES+1];
	static double vertexList[Square::EDGES][2];

	static int GetIndex(const double values[Square::CORNERS],const double& iso);
	static int IsAmbiguous(const double v[Square::CORNERS],const double& isoValue);
	static int AddEdges(const double v[Square::CORNERS],const double& isoValue,Edge* edges);
	static int AddEdgeIndices(const double v[Square::CORNERS],const double& isoValue,int* edges);
};

class MarchingCubes{
	static double Interpolate(const double& v1,const double& v2);
	static void SetVertex(const int& e,const double values[Cube::CORNERS],const double& iso);
	static int GetFaceIndex(const double values[Cube::CORNERS],const double& iso,const int& faceIndex);

	static float Interpolate(const float& v1,const float& v2);
	static void SetVertex(const int& e,const float values[Cube::CORNERS],const float& iso);
	static int GetFaceIndex(const float values[Cube::CORNERS],const float& iso,const int& faceIndex);

	static int GetFaceIndex(const int& mcIndex,const int& faceIndex);
public:
	const static int MAX_TRIANGLES=5;
	static const int edgeMask[1<<Cube::CORNERS];
	static const int triangles[1<<Cube::CORNERS][3*MAX_TRIANGLES+1];
	static const int cornerMap[Cube::CORNERS];
	static double vertexList[Cube::EDGES][3];

	static int AddTriangleIndices(const int& mcIndex,int* triangles);

	static int GetIndex(const double values[Cube::CORNERS],const double& iso);
	static int IsAmbiguous(const double v[Cube::CORNERS],const double& isoValue,const int& faceIndex);
	static int HasRoots(const double v[Cube::CORNERS],const double& isoValue);
	static int HasRoots(const double v[Cube::CORNERS],const double& isoValue,const int& faceIndex);
	static int AddTriangles(const double v[Cube::CORNERS],const double& isoValue,Triangle* triangles);
	static int AddTriangleIndices(const double v[Cube::CORNERS],const double& isoValue,int* triangles);

	static int GetIndex(const float values[Cube::CORNERS],const float& iso);
	static int IsAmbiguous(const float v[Cube::CORNERS],const float& isoValue,const int& faceIndex);
	static int HasRoots(const float v[Cube::CORNERS],const float& isoValue);
	static int HasRoots(const float v[Cube::CORNERS],const float& isoValue,const int& faceIndex);
	static int AddTriangles(const float v[Cube::CORNERS],const float& isoValue,Triangle* triangles);
	static int AddTriangleIndices(const float v[Cube::CORNERS],const float& isoValue,int* triangles);

	static int IsAmbiguous(const int& mcIndex,const int& faceIndex);
	static int HasRoots(const int& mcIndex);
	static int HasFaceRoots(const int& mcIndex,const int& faceIndex);
	static int HasEdgeRoots(const int& mcIndex,const int& edgeIndex);
};


/*=========================================================================
MemoryUseage.h
=========================================================================*/
class MemoryInfo{
public:
	size_t TotalPhysicalMemory;
	size_t FreePhysicalMemory;
	size_t TotalSwapSpace;
	size_t FreeSwapSpace;
	size_t TotalVirtualAddressSpace;
	size_t FreeVirtualAddressSpace;
	size_t PageSize;

	void set(void){
		MEMORYSTATUSEX Mem;
		SYSTEM_INFO Info;
		ZeroMemory( &Mem, sizeof(Mem));
		ZeroMemory( &Info, sizeof(Info)); 
		Mem.dwLength = sizeof(Mem);
		::GlobalMemoryStatusEx( &Mem );
		::GetSystemInfo( &Info );

		TotalPhysicalMemory = (size_t)Mem.ullTotalPhys;
		FreePhysicalMemory = (size_t)Mem.ullAvailPhys;
		TotalSwapSpace = (size_t)Mem.ullTotalPageFile;
		FreeSwapSpace = (size_t)Mem.ullAvailPageFile;
		TotalVirtualAddressSpace = (size_t)Mem.ullTotalVirtual;
		FreeVirtualAddressSpace = (size_t)Mem.ullAvailVirtual;
		PageSize = (size_t)Info.dwPageSize;
	}
	size_t usage(void) const {return TotalVirtualAddressSpace-FreeVirtualAddressSpace;}

	static size_t Usage(void){
		MEMORY_BASIC_INFORMATION mbi; 
		size_t      dwMemUsed = 0; 
		PVOID      pvAddress = 0; 


		memset(&mbi, 0, sizeof(MEMORY_BASIC_INFORMATION)); 
		while(VirtualQuery(pvAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == sizeof(MEMORY_BASIC_INFORMATION)){ 
			if(mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE){dwMemUsed += mbi.RegionSize;}
			pvAddress = ((BYTE*)mbi.BaseAddress) + mbi.RegionSize; 
		} 
		return dwMemUsed; 
	} 
};

/*=========================================================================
Vector.h
=========================================================================*/
template<class T>
class Vector
{
public:
	Vector();
	Vector( const Vector<T>& V );
	Vector( size_t N );
	Vector( size_t N, T* pV );
	~Vector();

	const T& operator () (size_t i) const;
	T& operator () (size_t i);
	const T& operator [] (size_t i) const;
	T& operator [] (size_t i);

	void SetZero();

	size_t Dimensions() const;
	void Resize( size_t N );

	Vector operator * (const T& A) const;
	Vector operator / (const T& A) const;
	Vector operator - (const Vector& V) const;
	Vector operator + (const Vector& V) const;

	Vector& operator *= (const T& A);
	Vector& operator /= (const T& A);
	Vector& operator += (const Vector& V);
	Vector& operator -= (const Vector& V);

	Vector& AddScaled(const Vector& V,const T& scale);
	Vector& SubtractScaled(const Vector& V,const T& scale);
	static void Add(const Vector& V1,const T& scale1,const Vector& V2,const T& scale2,Vector& Out);
	static void Add(const Vector& V1,const T& scale1,const Vector& V2,Vector& Out);

	Vector operator - () const;

	Vector& operator = (const Vector& V);

	T Dot( const Vector& V ) const;

	T Length() const;

	T Norm( size_t Ln ) const;
	void Normalize();

	T* m_pV;
protected:
	size_t m_N;

};

template<class T,int Dim>
class NVector
{
public:
	NVector();
	NVector( const NVector& V );
	NVector( size_t N );
	NVector( size_t N, T* pV );
	~NVector();

	const T* operator () (size_t i) const;
	T* operator () (size_t i);
	const T* operator [] (size_t i) const;
	T* operator [] (size_t i);

	void SetZero();

	size_t Dimensions() const;
	void Resize( size_t N );

	NVector operator * (const T& A) const;
	NVector operator / (const T& A) const;
	NVector operator - (const NVector& V) const;
	NVector operator + (const NVector& V) const;

	NVector& operator *= (const T& A);
	NVector& operator /= (const T& A);
	NVector& operator += (const NVector& V);
	NVector& operator -= (const NVector& V);

	NVector& AddScaled(const NVector& V,const T& scale);
	NVector& SubtractScaled(const NVector& V,const T& scale);
	static void Add(const NVector& V1,const T& scale1,const NVector& V2,const T& scale2,NVector& Out);
	static void Add(const NVector& V1,const T& scale1,const NVector& V2,				NVector& Out);

	NVector operator - () const;

	NVector& operator = (const NVector& V);

	T Dot( const NVector& V ) const;

	T Length() const;

	T Norm( size_t Ln ) const;
	void Normalize();

	T* m_pV;
protected:
	size_t m_N;

};


/*=========================================================================
SparseMatrix.h
=========================================================================*/
template <class T>
struct MatrixEntry
{
	MatrixEntry( void )		{ N =-1; Value = 0; }
	MatrixEntry( int i )	{ N = i; Value = 0; }
	int N;
	T Value;
};
template <class T,int Dim>
struct NMatrixEntry
{
	NMatrixEntry( void )		{ N =-1; memset(Value,0,sizeof(T)*Dim); }
	NMatrixEntry( int i )	{ N = i; memset(Value,0,sizeof(T)*Dim); }
	int N;
	T Value[Dim];
};

template<class T> class SparseMatrix
{
private:
	static int UseAlloc;
public:
	static Allocator<MatrixEntry<T> > Allocator;
	static int UseAllocator(void);
	static void SetAllocator(const int& blockSize);

	int rows;
	int* rowSizes;
	MatrixEntry<T>** m_ppElements;

	SparseMatrix();
	SparseMatrix( int rows );
	void Resize( int rows );
	void SetRowSize( int row , int count );
	int Entries(void);

	SparseMatrix( const SparseMatrix& M );
	~SparseMatrix();

	void SetZero();
	void SetIdentity();

	SparseMatrix<T>& operator = (const SparseMatrix<T>& M);

	SparseMatrix<T> operator * (const T& V) const;
	SparseMatrix<T>& operator *= (const T& V);


	SparseMatrix<T> operator * (const SparseMatrix<T>& M) const;
	SparseMatrix<T> Multiply( const SparseMatrix<T>& M ) const;
	SparseMatrix<T> MultiplyTranspose( const SparseMatrix<T>& Mt ) const;

	template<class T2>
	Vector<T2> operator * (const Vector<T2>& V) const;
	template<class T2>
	Vector<T2> Multiply( const Vector<T2>& V ) const;
	template<class T2>
	void Multiply( const Vector<T2>& In, Vector<T2>& Out ) const;


	SparseMatrix<T> Transpose() const;

	static int Solve			(const SparseMatrix<T>& M,const Vector<T>& b,const int& iters,Vector<T>& solution,const T eps=1e-8);

	template<class T2>
	static int SolveSymmetric	(const SparseMatrix<T>& M,const Vector<T2>& b,const int& iters,Vector<T2>& solution,const T2 eps=1e-8,const int& reset=1);

};
template<class T,int Dim> class SparseNMatrix
{
private:
	static int UseAlloc;
public:
	static Allocator<NMatrixEntry<T,Dim> > Allocator;
	static int UseAllocator(void);
	static void SetAllocator(const int& blockSize);

	int rows;
	int* rowSizes;
	NMatrixEntry<T,Dim>** m_ppElements;

	SparseNMatrix();
	SparseNMatrix( int rows );
	void Resize( int rows );
	void SetRowSize( int row , int count );
	int Entries(void);

	SparseNMatrix( const SparseNMatrix& M );
	~SparseNMatrix();

	SparseNMatrix& operator = (const SparseNMatrix& M);

	SparseNMatrix  operator *  (const T& V) const;
	SparseNMatrix& operator *= (const T& V);

	template<class T2>
	NVector<T2,Dim> operator * (const Vector<T2>& V) const;
	template<class T2>
	Vector<T2> operator * (const NVector<T2,Dim>& V) const;
};



template <class T>
class SparseSymmetricMatrix : public SparseMatrix<T>{
public:

  template<class T2>
	Vector<T2> operator * (const Vector<T2>& V) const;
	template<class T2>
	Vector<T2> Multiply( const Vector<T2>& V ) const;
	template<class T2>
	void Multiply( const Vector<T2>& In, Vector<T2>& Out ) const;

	template<class T2>
	static int Solve(const SparseSymmetricMatrix<T>& M,const Vector<T2>& b,const int& iters,Vector<T2>& solution,const T2 eps=1e-8,const int& reset=1);

	template<class T2>
	static int Solve(const SparseSymmetricMatrix<T>& M,const Vector<T>& diagonal,const Vector<T2>& b,const int& iters,Vector<T2>& solution,const T2 eps=1e-8,const int& reset=1);
};


/*=========================================================================
Octree.h
=========================================================================*/
template<class NodeData,class Real=float>
class OctNode
{
private:
	static int UseAlloc;

	class AdjacencyCountFunction{
	public:
		int count;
		void Function(const OctNode<NodeData,Real>* node1,const OctNode<NodeData,Real>* node2);
	};
	template<class NodeAdjacencyFunction>
	void __processNodeFaces(OctNode* node,NodeAdjacencyFunction* F,const int& cIndex1,const int& cIndex2,const int& cIndex3,const int& cIndex4);
	template<class NodeAdjacencyFunction>
	void __processNodeEdges(OctNode* node,NodeAdjacencyFunction* F,const int& cIndex1,const int& cIndex2);
	template<class NodeAdjacencyFunction>
	void __processNodeNodes(OctNode* node,NodeAdjacencyFunction* F);
	template<class NodeAdjacencyFunction>
	static void __ProcessNodeAdjacentNodes(const int& dx,const int& dy,const int& dz,OctNode* node1,const int& radius1,OctNode* node2,const int& radius2,const int& cWidth2,NodeAdjacencyFunction* F);
	template<class TerminatingNodeAdjacencyFunction>
	static void __ProcessTerminatingNodeAdjacentNodes(const int& dx,const int& dy,const int& dz,OctNode* node1,const int& radius1,OctNode* node2,const int& radius2,const int& cWidth2,TerminatingNodeAdjacencyFunction* F);
	template<class PointAdjacencyFunction>
	static void __ProcessPointAdjacentNodes(const int& dx,const int& dy,const int& dz,OctNode* node2,const int& radius2,const int& cWidth2,PointAdjacencyFunction* F);
	template<class NodeAdjacencyFunction>
	static void __ProcessFixedDepthNodeAdjacentNodes(const int& dx,const int& dy,const int& dz,OctNode* node1,const int& radius1,OctNode* node2,const int& radius2,const int& cWidth2,const int& depth,NodeAdjacencyFunction* F);
	template<class NodeAdjacencyFunction>
	static void __ProcessMaxDepthNodeAdjacentNodes(const int& dx,const int& dy,const int& dz,OctNode* node1,const int& radius1,OctNode* node2,const int& radius2,const int& cWidth2,const int& depth,NodeAdjacencyFunction* F);

	// This is made private because the division by two has been pulled out.
	static inline int Overlap(const int& c1,const int& c2,const int& c3,const int& dWidth);
	inline static int ChildOverlap(const int& dx,const int& dy,const int& dz,const int& d,const int& cRadius2);

	const OctNode* __faceNeighbor(const int& dir,const int& off) const;
	const OctNode* __edgeNeighbor(const int& o,const int i[2],const int idx[2]) const;
	OctNode* __faceNeighbor(const int& dir,const int& off,const int& forceChildren);
	OctNode* __edgeNeighbor(const int& o,const int i[2],const int idx[2],const int& forceChildren);
public:
	static const int DepthShift,OffsetShift,OffsetShift1,OffsetShift2,OffsetShift3;
	static const int DepthMask,OffsetMask;

	static Allocator<OctNode> Allocator;
	static int UseAllocator(void);
	static void SetAllocator(int blockSize);

	OctNode* parent;
	OctNode* children;
	short d,off[3];
	NodeData nodeData;


	OctNode(void);
	~OctNode(void);
	int initChildren(void);

	void depthAndOffset(int& depth,int offset[3]) const; 
	int depth(void) const;
	static inline void DepthAndOffset(const long long& index,int& depth,int offset[3]);
	static inline void CenterAndWidth(const long long& index,Point3D<Real>& center,Real& width);
	static inline int Depth(const long long& index);
	static inline void Index(const int& depth,const int offset[3],short& d,short off[3]);
	void centerAndWidth(Point3D<Real>& center,Real& width) const;

	int leaves(void) const;
	int maxDepthLeaves(const int& maxDepth) const;
	int nodes(void) const;
	int maxDepth(void) const;

	const OctNode* root(void) const;

	const OctNode* nextLeaf(const OctNode* currentLeaf=NULL) const;
	OctNode* nextLeaf(OctNode* currentLeaf=NULL);
	const OctNode* nextNode(const OctNode* currentNode=NULL) const;
	OctNode* nextNode(OctNode* currentNode=NULL);
	const OctNode* nextBranch(const OctNode* current) const;
	OctNode* nextBranch(OctNode* current);

	void setFullDepth(const int& maxDepth);

	void printLeaves(void) const;
	void printRange(void) const;

	template<class NodeAdjacencyFunction>
	void processNodeFaces(OctNode* node,NodeAdjacencyFunction* F,const int& fIndex,const int& processCurrent=1);
	template<class NodeAdjacencyFunction>
	void processNodeEdges(OctNode* node,NodeAdjacencyFunction* F,const int& eIndex,const int& processCurrent=1);
	template<class NodeAdjacencyFunction>
	void processNodeCorners(OctNode* node,NodeAdjacencyFunction* F,const int& cIndex,const int& processCurrent=1);
	template<class NodeAdjacencyFunction>
	void processNodeNodes(OctNode* node,NodeAdjacencyFunction* F,const int& processCurrent=1);
	
	template<class NodeAdjacencyFunction>
	static void ProcessNodeAdjacentNodes(const int& maxDepth,OctNode* node1,const int& width1,OctNode* node2,const int& width2,NodeAdjacencyFunction* F,const int& processCurrent=1);
	template<class NodeAdjacencyFunction>
	static void ProcessNodeAdjacentNodes(const int& dx,const int& dy,const int& dz,OctNode* node1,const int& radius1,OctNode* node2,const int& radius2,const int& width2,NodeAdjacencyFunction* F,const int& processCurrent=1);
	template<class TerminatingNodeAdjacencyFunction>
	static void ProcessTerminatingNodeAdjacentNodes(const int& maxDepth,OctNode* node1,const int& width1,OctNode* node2,const int& width2,TerminatingNodeAdjacencyFunction* F,const int& processCurrent=1);
	template<class TerminatingNodeAdjacencyFunction>
	static void ProcessTerminatingNodeAdjacentNodes(const int& dx,const int& dy,const int& dz,OctNode* node1,const int& radius1,OctNode* node2,const int& radius2,const int& width2,TerminatingNodeAdjacencyFunction* F,const int& processCurrent=1);
	template<class PointAdjacencyFunction>
	static void ProcessPointAdjacentNodes(const int& maxDepth,const int center1[3],OctNode* node2,const int& width2,PointAdjacencyFunction* F,const int& processCurrent=1);
	template<class PointAdjacencyFunction>
	static void ProcessPointAdjacentNodes(const int& dx,const int& dy,const int& dz,OctNode* node2,const int& radius2,const int& width2,PointAdjacencyFunction* F,const int& processCurrent=1);
	template<class NodeAdjacencyFunction>
	static void ProcessFixedDepthNodeAdjacentNodes(const int& maxDepth,OctNode* node1,const int& width1,OctNode* node2,const int& width2,const int& depth,NodeAdjacencyFunction* F,const int& processCurrent=1);
	template<class NodeAdjacencyFunction>
	static void ProcessFixedDepthNodeAdjacentNodes(const int& dx,const int& dy,const int& dz,OctNode* node1,const int& radius1,OctNode* node2,const int& radius2,const int& width2,const int& depth,NodeAdjacencyFunction* F,const int& processCurrent=1);
	template<class NodeAdjacencyFunction>
	static void ProcessMaxDepthNodeAdjacentNodes(const int& maxDepth,OctNode* node1,const int& width1,OctNode* node2,const int& width2,const int& depth,NodeAdjacencyFunction* F,const int& processCurrent=1);
	template<class NodeAdjacencyFunction>
	static void ProcessMaxDepthNodeAdjacentNodes(const int& dx,const int& dy,const int& dz,OctNode* node1,const int& radius1,OctNode* node2,const int& radius2,const int& width2,const int& depth,NodeAdjacencyFunction* F,const int& processCurrent=1);

	static int CornerIndex(const Point3D<Real>& center,const Point3D<Real> &p);

	OctNode* faceNeighbor(const int& faceIndex,const int& forceChildren=0);
	const OctNode* faceNeighbor(const int& faceIndex) const;
	OctNode* edgeNeighbor(const int& edgeIndex,const int& forceChildren=0);
	const OctNode* edgeNeighbor(const int& edgeIndex) const;
	OctNode* cornerNeighbor(const int& cornerIndex,const int& forceChildren=0);
	const OctNode* cornerNeighbor(const int& cornerIndex) const;

	OctNode* getNearestLeaf(const Point3D<Real>& p);
	const OctNode* getNearestLeaf(const Point3D<Real>& p) const;

	static int CommonEdge(const OctNode* node1,const int& eIndex1,const OctNode* node2,const int& eIndex2);
	static int CompareForwardDepths(const void* v1,const void* v2);
	static int CompareForwardPointerDepths(const void* v1,const void* v2);
	static int CompareBackwardDepths(const void* v1,const void* v2);
	static int CompareBackwardPointerDepths(const void* v1,const void* v2);


	template<class NodeData2>
	OctNode& operator = (const OctNode<NodeData2,Real>& node);

	static inline int Overlap2(const int &depth1,const int offSet1[DIMENSION],const Real& multiplier1,const int &depth2,const int offSet2[DIMENSION],const Real& multiplier2);

/*
	int write(const char* fileName) const;
	int write(FILE* fp) const;
	int read(const char* fileName);
	int read(FILE* fp);
*/
	class Neighbors{
	public:
		OctNode* neighbors[3][3][3];
		Neighbors(void);
		void clear(void);
	};
	class NeighborKey{
	public:
		Neighbors* neighbors;

		NeighborKey(void);
		~NeighborKey(void);

		void set(const int& depth);
		Neighbors& setNeighbors(OctNode* node);
		Neighbors& getNeighbors(OctNode* node);
	};
	class Neighbors2{
	public:
		const OctNode* neighbors[3][3][3];
		Neighbors2(void);
		void clear(void);
	};
	class NeighborKey2{
	public:
		Neighbors2* neighbors;

		NeighborKey2(void);
		~NeighborKey2(void);

		void set(const int& depth);
		Neighbors2& getNeighbors(const OctNode* node);
	};

	void centerIndex(const int& maxDepth,int index[DIMENSION]) const;
	int width(const int& maxDepth) const;
};


/*=========================================================================
MultiGridOctreeData.h
=========================================================================*/
typedef float Real;
typedef float FunctionDataReal;
typedef OctNode<class TreeNodeData,Real> TreeOctNode;

class RootInfo{
public:
	const TreeOctNode* node;
	int edgeIndex;
	long long key;
};

class VertexData{
public:
	static long long EdgeIndex(const TreeOctNode* node,const int& eIndex,const int& maxDepth,int index[DIMENSION]);
	static long long EdgeIndex(const TreeOctNode* node,const int& eIndex,const int& maxDepth);
	static long long FaceIndex(const TreeOctNode* node,const int& fIndex,const int& maxDepth,int index[DIMENSION]);
	static long long FaceIndex(const TreeOctNode* node,const int& fIndex,const int& maxDepth);
	static long long CornerIndex(const int& depth,const int offSet[DIMENSION],const int& cIndex,const int& maxDepth,int index[DIMENSION]);
	static long long CornerIndex(const TreeOctNode* node,const int& cIndex,const int& maxDepth,int index[DIMENSION]);
	static long long CornerIndex(const TreeOctNode* node,const int& cIndex,const int& maxDepth);
	static long long CenterIndex(const int& depth,const int offSet[DIMENSION],const int& maxDepth,int index[DIMENSION]);
	static long long CenterIndex(const TreeOctNode* node,const int& maxDepth,int index[DIMENSION]);
	static long long CenterIndex(const TreeOctNode* node,const int& maxDepth);
};
class SortedTreeNodes{
public:
	TreeOctNode** treeNodes;
	int *nodeCount;
	int maxDepth;
	SortedTreeNodes(void);
	~SortedTreeNodes(void);
	void set(TreeOctNode& root,const int& setIndex);
};

class TreeNodeData{
public:
	static int UseIndex;
	union{
		int mcIndex;
		struct{
			int nodeIndex;
			Real centerWeightContribution;
		};
	};
	Real value;

	TreeNodeData(void);
	~TreeNodeData(void);
};

template<int Degree>
class Octree{
	TreeOctNode::NeighborKey neighborKey;	
	TreeOctNode::NeighborKey2 neighborKey2;

	Real radius;
	int width;
	void setNodeIndices(TreeOctNode& tree,int& idx);
	Real GetDotProduct(const int index[DIMENSION]) const;
	Real GetLaplacian(const int index[DIMENSION]) const;
	Real GetDivergence(const int index[DIMENSION],const Point3D<Real>& normal) const;

	class DivergenceFunction{
	public:
		Point3D<Real> normal;
		Octree<Degree>* ot;
		int index[DIMENSION],scratch[DIMENSION];
		void Function(TreeOctNode* node1,const TreeOctNode* node2);
	};

	class LaplacianProjectionFunction{
	public:
		double value;
		Octree<Degree>* ot;
		int index[DIMENSION],scratch[DIMENSION];
		void Function(TreeOctNode* node1,const TreeOctNode* node2);
	};
	class LaplacianMatrixFunction{
	public:
		int x2,y2,z2,d2;
		Octree<Degree>* ot;
		int index[DIMENSION],scratch[DIMENSION];
		int elementCount,offset;
		MatrixEntry<float>* rowElements;
		int Function(const TreeOctNode* node1,const TreeOctNode* node2);
	};
	class RestrictedLaplacianMatrixFunction{
	public:
		int depth,offset[3];
		Octree<Degree>* ot;
		Real radius;
		int index[DIMENSION],scratch[DIMENSION];
		int elementCount;
		MatrixEntry<float>* rowElements;
		int Function(const TreeOctNode* node1,const TreeOctNode* node2);
	};

	///////////////////////////
	// Evaluation Functions  //
	///////////////////////////
	class PointIndexValueFunction{
	public:
		int res2;
		FunctionDataReal* valueTables;
		int index[DIMENSION];
		Real value;
		void Function(const TreeOctNode* node);
	};
	class PointIndexValueAndNormalFunction{
	public:
		int res2;
		FunctionDataReal* valueTables;
		FunctionDataReal* dValueTables;
		Real value;
		Point3D<Real> normal;
		int index[DIMENSION];
		void Function(const TreeOctNode* node);
	};

	class AdjacencyCountFunction{
	public:
		int adjacencyCount;
		void Function(const TreeOctNode* node1,const TreeOctNode* node2);
	};
	class AdjacencySetFunction{
	public:
		int *adjacencies,adjacencyCount;
		void Function(const TreeOctNode* node1,const TreeOctNode* node2);
	};

	class RefineFunction{
	public:
		int depth;
		void Function(TreeOctNode* node1,const TreeOctNode* node2);
	};
	class FaceEdgesFunction{
	public:
		int fIndex,maxDepth;
		std::vector<std::pair<long long,long long> >* edges;
		hash_map<long long,std::pair<RootInfo,int> >* vertexCount;
		void Function(const TreeOctNode* node1,const TreeOctNode* node2);
	};

	int SolveFixedDepthMatrix(const int& depth,const SortedTreeNodes& sNodes);
	int SolveFixedDepthMatrix(const int& depth,const int& startingDepth,const SortedTreeNodes& sNodes);

	int GetFixedDepthLaplacian(SparseSymmetricMatrix<float>& matrix,const int& depth,const SortedTreeNodes& sNodes);
	int GetRestrictedFixedDepthLaplacian(SparseSymmetricMatrix<float>& matrix,const int& depth,const int* entries,const int& entryCount,const TreeOctNode* rNode,const Real& radius,const SortedTreeNodes& sNodes);

	void SetIsoSurfaceCorners(const Real& isoValue,const int& subdivisionDepth,const int& fullDepthIso);
	static int IsBoundaryFace(const TreeOctNode* node,const int& faceIndex,const int& subdivideDepth);
	static int IsBoundaryEdge(const TreeOctNode* node,const int& edgeIndex,const int& subdivideDepth);
	static int IsBoundaryEdge(const TreeOctNode* node,const int& dir,const int& x,const int& y,const int& subidivideDepth);
	void PreValidate(const Real& isoValue,const int& maxDepth,const int& subdivideDepth);
	void PreValidate(TreeOctNode* node,const Real& isoValue,const int& maxDepth,const int& subdivideDepth);
	void Validate(TreeOctNode* node,const Real& isoValue,const int& maxDepth,const int& fullDepthIso,const int& subdivideDepth);
	void Validate(TreeOctNode* node,const Real& isoValue,const int& maxDepth,const int& fullDepthIso);
	void Subdivide(TreeOctNode* node,const Real& isoValue,const int& maxDepth);

	int SetBoundaryMCRootPositions(const int& sDepth,const Real& isoValue,
		hash_map<long long,int>& boundaryRoots,hash_map<long long,std::pair<Real,Point3D<Real> > >& boundaryNormalHash,CoredMeshData* mesh,const int& nonLinearFit);
	int SetMCRootPositions(TreeOctNode* node,const int& sDepth,const Real& isoValue,
		hash_map<long long,int>& boundaryRoots,hash_map<long long,int>* interiorRoots,
		hash_map<long long,std::pair<Real,Point3D<Real> > >& boundaryNormalHash,hash_map<long long,std::pair<Real,Point3D<Real> > >* interiorNormalHash,
		std::vector<Point3D<float> >* interiorPositions,
		CoredMeshData* mesh,const int& nonLinearFit);

	int GetMCIsoTriangles(TreeOctNode* node,CoredMeshData* mesh,hash_map<long long,int>& boundaryRoots,
		hash_map<long long,int>* interiorRoots,std::vector<Point3D<float> >* interiorPositions,const int& offSet,const int& sDepth);

	static int AddTriangles(CoredMeshData* mesh,std::vector<CoredPointIndex> edges[3],std::vector<Point3D<float> >* interiorPositions,const int& offSet);
	static int AddTriangles(CoredMeshData* mesh,std::vector<CoredPointIndex>& edges,std::vector<Point3D<float> >* interiorPositions,const int& offSet);
	void GetMCIsoEdges(TreeOctNode* node,hash_map<long long,int>& boundaryRoots,hash_map<long long,int>* interiorRoots,const int& sDepth,
		std::vector<std::pair<long long,long long> >& edges);
	static int GetEdgeLoops(std::vector<std::pair<long long,long long> >& edges,std::vector<std::vector<std::pair<long long,long long> > >& loops);
	static int InteriorFaceRootCount(const TreeOctNode* node,const int &faceIndex,const int& maxDepth);
	static int EdgeRootCount(const TreeOctNode* node,const int& edgeIndex,const int& maxDepth);
	int GetRoot(const RootInfo& ri,const Real& isoValue,const int& maxDepth,Point3D<Real> & position,hash_map<long long,std::pair<Real,Point3D<Real> > >& normalHash,
		Point3D<Real>* normal,const int& nonLinearFit);
	int GetRoot(const RootInfo& ri,const Real& isoValue,Point3D<Real> & position,hash_map<long long,std::pair<Real,Point3D<Real> > >& normalHash,const int& nonLinearFit);
	static int GetRootIndex(const TreeOctNode* node,const int& edgeIndex,const int& maxDepth,RootInfo& ri);
	static int GetRootIndex(const TreeOctNode* node,const int& edgeIndex,const int& maxDepth,const int& sDepth,RootInfo& ri);
	static int GetRootIndex(const long long& key,hash_map<long long,int>& boundaryRoots,hash_map<long long,int>* interiorRoots,CoredPointIndex& index);
	static int GetRootPair(const RootInfo& root,const int& maxDepth,RootInfo& pair);

	int NonLinearUpdateWeightContribution(TreeOctNode* node,const Point3D<Real>& position,const Real& weight=Real(1.0));
	Real NonLinearGetSampleWeight(TreeOctNode* node,const Point3D<Real>& position);
	void NonLinearGetSampleDepthAndWeight(TreeOctNode* node,const Point3D<Real>& position,const Real& samplesPerNode,Real& depth,Real& weight);
	int NonLinearSplatOrientedPoint(TreeOctNode* node,const Point3D<Real>& point,const Point3D<Real>& normal);
	void NonLinearSplatOrientedPoint(const Point3D<Real>& point,const Point3D<Real>& normal,const int& kernelDepth,const Real& samplesPerNode,const int& minDepth,const int& maxDepth);

	int HasNormals(TreeOctNode* node,const Real& epsilon);

	Real getCenterValue(const TreeOctNode* node);
	Real getCornerValue(const TreeOctNode* node,const int& corner);
	void getCornerValueAndNormal(const TreeOctNode* node,const int& corner,Real& value,Point3D<Real>& normal);
public:
	static double maxMemoryUsage;
	static double MemoryUsage(void);
	std::vector< Point3D<Real> >* normals;
	Real postNormalSmooth;
	TreeOctNode tree;
	FunctionData<Degree,FunctionDataReal> fData;
	Octree(void);

	void setFunctionData(const PPolynomial<Degree>& ReconstructionFunction,const int& maxDepth,const int& normalize,const Real& normalSmooth=-1);
	void finalize1(const int& refineNeighbors=-1);
	void finalize2(const int& refineNeighbors=-1);
	int setTree(const int& maxDepth,const int& kernelDepth,const Real& samplesPerNode,
		const Real& scaleFactor,Point3D<Real>& center,Real& scale,const int& resetSampleDepths,const int& useConfidence);

	void SetLaplacianWeights(void);
	void ClipTree(void);
	int LaplacianMatrixIteration(const int& subdivideDepth);

	Real GetIsoValue(void);
	void GetMCIsoTriangles(const Real& isoValue,CoredMeshData* mesh,const int& fullDepthIso=0,const int& nonLinearFit=1);
	void GetMCIsoTriangles(const Real& isoValue,const int& subdivideDepth,CoredMeshData* mesh,const int& fullDepthIso=0,const int& nonLinearFit=1);
};


#endif