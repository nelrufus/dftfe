// ---------------------------------------------------------------------
//
// Copyright (c) 2017-2018 The Regents of the University of Michigan and DFT-FE authors.
//
// This file is part of the DFT-FE code.
//
// The DFT-FE code is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE at
// the top level of the DFT-FE distribution.
//
// ---------------------------------------------------------------------
//

#ifndef dft_H_
#define dft_H_
#include <iostream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <complex>
#include <deque>

#include <headers.h>
#include <constants.h>
#include <constraintMatrixInfo.h>

#include <kohnShamDFTOperator.h>
#include <meshMovementAffineTransform.h>
#include <meshMovementGaussian.h>
#include <eigenSolver.h>
#include <chebyshevOrthogonalizedSubspaceIterationSolver.h>
#include <vselfBinsManager.h>
#include <dftParameters.h>
#include <triangulationManager.h>
#include <poissonSolverProblem.h>
#include <dealiiLinearSolver.h>
#include <kerkerSolverProblem.h>

#include <interpolation.h>
#include <xc.h>
#ifdef USE_PETSC
#include <petsc.h>
#include <slepceps.h>
#endif
#include <spglib.h>
#include <stdafx.h>

namespace dftfe {

  //
  //Initialize Namespace
  //
  using namespace dealii;



#ifndef DOXYGEN_SHOULD_SKIP_THIS

  struct orbital
  {
    unsigned int atomID;
    unsigned int waveID;
    unsigned int Z, n, l;
    int m;
    alglib::spline1dinterpolant* psi;
  };

  /* code that must be skipped by Doxygen */
  //forward declarations
  template <unsigned int T> class forceClass;
  template <unsigned int T> class symmetryClass;
  template <unsigned int T> class forceClass;
  template <unsigned int T> class geoOptIon;
  template <unsigned int T> class geoOptCell;

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

 /**
  * @brief This class is the primary interface location of all other parts of the DFT-FE code
  * for all steps involved in obtaining the Kohn-Sham DFT ground-state solution.
  *
  * @author Shiva Rudraraju, Phani Motamarri, Sambit Das
  */
  template <unsigned int FEOrder>
    class dftClass
    {
      template <unsigned int T>
	friend class kohnShamDFTOperatorClass;

      template <unsigned int T>
	friend class forceClass;

      template <unsigned int T>
	friend class geoOptIon;

      template <unsigned int T>
	friend class geoOptCell;

      template <unsigned int T>
	friend class symmetryClass;

    public:

      /**
       * @brief dftClass constructor
       *
       *  @param[in] mpi_comm_replica  mpi_communicator for domain decomposition parallelization
       *  @param[in] interpoolcomm  mpi_communicator for parallelization over k points
       *  @param[in] interBandGroupComm  mpi_communicator for parallelization over bands
       */
      dftClass(const MPI_Comm &mpi_comm_replica,
	       const MPI_Comm &interpoolcomm,
	       const MPI_Comm &interBandGroupComm);

      /**
       * @brief dftClass destructor
       */
      ~dftClass();

      /**
       * @brief atomic system pre-processing steps.
       *
       * Reads the coordinates of the atoms.
       * If periodic calculation, reads fractional coordinates of atoms in the unit-cell,
       * lattice vectors, kPoint quadrature rules to be used and also generates image atoms.
       * Also determines orbital-ordering
       */
      void set();

      /**
       * @brief Does KSDFT problem pre-processing steps including mesh generation calls.
       */
      void init(const unsigned int usePreviousGroundStateFields=0);

      /**
       * @brief Does KSDFT problem pre-processing steps but without remeshing.
       */
      void initNoRemesh(const bool updateImageKPoints = true,
		        const bool useSingleAtomSolution = false );

      /**
       * @brief Selects between only electronic field relaxation or combined electronic and geometry relaxation
       */
      void run();

      /**
       * @brief compute approximation to ground-state without solving the SCF iteration
       */
      void solveNoSCF();
      /**
       * @brief Kohn-Sham ground-state solve using SCF iteration
       */
      void solve(const bool computeForces=true);

      /**
       * @brief Number of Kohn-Sham eigen values to be computed
       */
      unsigned int d_numEigenValues;

      /**
       * @brief Number of Kohn-Sham eigen values to be computed in the Rayleigh-Ritz step
       * after spectrum splitting.
       */
      unsigned int d_numEigenValuesRR;

      /**
       * @brief Number of random wavefunctions
       */
      unsigned int d_nonAtomicWaveFunctions;

      void readkPointData();

      /**
       *@brief Get local dofs global indices real
       */
      const std::vector<dealii::types::global_dof_index> & getLocalDofIndicesReal() const;

      /**
       *@brief Get local dofs global indices imag
       */
      const std::vector<dealii::types::global_dof_index> & getLocalDofIndicesImag() const;

      /**
       *@brief Get local dofs local proc indices real
       */
      const std::vector<dealii::types::global_dof_index> & getLocalProcDofIndicesReal() const;

      /**
       *@brief Get local dofs local proc indices imag
       */
      const std::vector<dealii::types::global_dof_index> & getLocalProcDofIndicesImag() const;

      /**
       *@brief Get dealii constraint matrix involving periodic constraints and hanging node constraints in periodic
       *case else only hanging node constraints in non-periodic case
       */
      const ConstraintMatrix & getConstraintMatrixEigen() const;

      /**
       *@brief Get overloaded constraint matrix information involving periodic constraints and hanging node constraints in periodic
       *case else only hanging node constraints in non-periodic case (data stored in STL format)
       */
      const dftUtils::constraintMatrixInfo & getConstraintMatrixEigenDataInfo() const;


      /**
       *@brief Get matrix free data object
       */
      const MatrixFree<3,double> & getMatrixFreeData() const;


      /** @brief Updates atom positions, remeshes/moves mesh and calls appropriate reinits.
       *
       *  Function to update the atom positions and mesh based on the provided displacement input.
       *  Depending on the maximum displacement magnitude this function decides wether to do auto remeshing
       *  or move mesh using Gaussian functions. Additionaly this function also wraps the atom position across the
       *  periodic boundary if the atom moves across it.
       *
       *  @param[in] globalAtomsDisplacements vector containing the displacements (from current position) of all atoms (global).
       *  @return void.
       */
      void updateAtomPositionsAndMoveMesh(const std::vector<Tensor<1,3,double> > & globalAtomsDisplacements,
	                                  double maxDisplacement,
					  const bool useSingleAtomSolutions=false);


      /**
       * @brief writes the current domain bounding vectors and atom coordinates to files, which are required for
       * geometry relaxation restart
       */
      void writeDomainAndAtomCoordinates();


    private:

      /**
       * @brief generate image charges and update k point cartesian coordinates based
       * on current lattice vectors
       */
      void initImageChargesUpdateKPoints(bool flag=true);

      /**
       */
      void initPsiAndRhoFromPreviousGroundStatePsi(std::vector<std::vector<vectorType>> eigenVectors);


      /**
       * @brief interpolate rho quadrature data on current mesh from the ground state rho on previous mesh.
       * This is used whenver the mesh is changed due to atom movement.
       */
      void initRhoFromPreviousGroundStateRho();

      /**
       * @brief update previous mesh data structures which are required for interpolating wfc and
       * density during geometry optimization.
       */
      void updatePrevMeshDataStructures();

      /**
       *@brief project ground state electron density from previous mesh into
       * the new mesh to be used as initial guess for the new ground state solve
       */
      void projectPreviousGroundStateRho();

      /**
       *@brief save triangulation information and rho quadrature data to checkpoint file for restarts
       */
      void saveTriaInfoAndRhoData();

      /**
       *@brief load triangulation information rho quadrature data from checkpoint file for restarted run
       */
      void loadTriaInfoAndRhoData();

      void generateMPGrid();
      void writeMesh(std::string meshFileName);

      /// creates datastructures related to periodic image charges
      void generateImageCharges(const double pspCutOff,
	                        std::vector<int> & imageIds,
				std::vector<double> & imageCharges,
				std::vector<std::vector<double> > & imagePositions,
				std::vector<std::vector<int> > & globalChargeIdToImageIdMap);

      void determineOrbitalFilling();

      //
      //generate mesh using a-posteriori error estimates
      //
      void aposterioriMeshGenerate();
      dataTypes::number computeTraceXtHX(unsigned int numberWaveFunctionsEstimate);
      double computeTraceXtKX(unsigned int numberWaveFunctionsEstimate);


      /**
       *@brief  moves the triangulation vertices using Gaussians such that the all atoms are on triangulation vertices
       */
      void moveMeshToAtoms(Triangulation<3,3> & triangulationMove,
		           Triangulation<3,3> & triangulationSerial,
			   bool reuseFlag = false,
			   bool moveSubdivided = false);

      /**
       * Initializes the guess of electron-density and single-atom wavefunctions on the mesh,
       * maps finite-element nodes to given atomic positions,
       * initializes pseudopotential files and exchange-correlation functionals to be used
       * based on user-choice.
       * In periodic problems, periodic faces are mapped here. Further finite-element nodes
       * to be pinned for solving the Poisson problem electro-static potential is set here
       */
      void initUnmovedTriangulation(parallel::distributed::Triangulation<3> & triangulation);
      void initBoundaryConditions();
      void initElectronicFields(const unsigned int usePreviousGroundStateFields=0);
      void initPseudoPotentialAll();

      /**
       * create a dofHandler containing finite-element interpolating polynomial twice of the original polynomial
       * required for Kerker mixing
       * and initialize various objects related to this refined dofHandler
       */
      void createpRefinedDofHandler(parallel::distributed::Triangulation<3> & triangulation);
      void initpRefinedObjects();

      /**
       *@brief interpolate nodal data to quadrature data using FEEvaluation
       *
       *@param[in] matrixFreeData matrix free data object
       *@param[in] nodalField nodal data to be interpolated
       *@param[out] quadratureValueData to be computed at quadrature points
       *@param[out] quadratureGradValueData to be computed at quadrature points
       *@param[in] isEvaluateGradData denotes a flag to evaluate gradients or not
       */
      void interpolateNodalDataToQuadratureData(dealii::MatrixFree<3,double> & matrixFreeData,
						vectorType & nodalField,
						std::map<dealii::CellId, std::vector<double> > & quadratureValueData,
						std::map<dealii::CellId, std::vector<double> > & quadratureGradValueData,
						const bool isEvaluateGradData);

     /**
      *@brief Finds the global dof ids of the nodes containing atoms.
      *
      * @param[in] dofHandler
      * @param[out] atomNodeIdToChargeValueMap local map of global dof id to atom charge id
      */
      void locateAtomCoreNodes(const dealii::DoFHandler<3> & _dofHandler,
	                       std::map<dealii::types::global_dof_index, double> & atomNodeIdToChargeValueMap);

     /**
      *@brief Sets homogeneous dirichlet boundary conditions on a node farthest from
      * all atoms (pinned node). This is only done in case of periodic boundary conditions
      * to get an unique solution to the total electrostatic potential problem.
      *
      * @param[in] dofHandler
      * @param[in] constraintMatrixBase base ConstraintMatrix object
      * @param[out] constraintMatrix ConstraintMatrix object with homogeneous
      * Dirichlet boundary condition entries added
      */
      void locatePeriodicPinnedNodes(const dealii::DoFHandler<3> & _dofHandler,
	                             const dealii::ConstraintMatrix & constraintMatrixBase,
	                             dealii::ConstraintMatrix & constraintMatrix);
      void initRho();
      void computeRhoInitialGuessFromPSI(std::vector<std::vector<vectorType>> eigenVectors);
      void clearRhoData();

      /**
       *@brief computes nodal electron-density from cell quadrature data using project function of dealii
       */
      void computeNodalRhoFromQuadData();


      /**
       *@brief computes density quadratrue dat from wavefunctions
       */
      void computeRhoFromPSI
		    (std::map<dealii::CellId, std::vector<double> > * _rhoValues,
		     std::map<dealii::CellId, std::vector<double> > * _gradRhoValues,
		     std::map<dealii::CellId, std::vector<double> > * _rhoValuesSpinPolarized,
		     std::map<dealii::CellId, std::vector<double> > * _gradRhoValuesSpinPolarized,
		     const bool isEvaluateGradRho,
		     const bool isConsiderSpectrumSplitting,
		     const bool lobattoNodesFlag = false);

      /**
       *@brief computes density nodal data from wavefunctions
       */
      void computeRhoNodalFromPSI(bool isConsiderSpectrumSplitting);


      /**
       *@brief sums rho cell quadratrure data from  inter communicator
       */
      void sumRhoData(std::map<dealii::CellId, std::vector<double> > * _rhoValues,
	              std::map<dealii::CellId, std::vector<double> > * _gradRhoValues,
	              std::map<dealii::CellId, std::vector<double> > * _rhoValuesSpinPolarized,
		      std::map<dealii::CellId, std::vector<double> > * _gradRhoValuesSpinPolarized,
		      const bool isGradRhoDataPresent,
		      const MPI_Comm &interComm);

      /**
       *@brief resize and allocate table storage for rho cell quadratrue data
       */
      void resizeAndAllocateRhoTableStorage
			    (std::deque<std::map<dealii::CellId,std::vector<double> >> & rhoVals,
			     std::deque<std::map<dealii::CellId,std::vector<double> >> & gradRhoVals,
			     std::deque<std::map<dealii::CellId,std::vector<double> >> & rhoValsSpinPolarized,
			     std::deque<std::map<dealii::CellId,std::vector<double> >> & gradRhoValsSpinPolarized);

      void noRemeshRhoDataInit();
      void readPSI();
      void readPSIRadialValues();
      void loadPSIFiles(unsigned int Z, unsigned int n, unsigned int l, unsigned int & flag);
      void initLocalPseudoPotential(const DoFHandler<3> & _dofHandler,
	   const dealii::QGauss<3> & _quadrature,
	   std::map<dealii::CellId, std::vector<double> > & _pseudoValues,
	   std::map<dealii::CellId, std::vector<double> > & _gradPseudoValues,
	   std::map<unsigned int,std::map<dealii::CellId, std::vector<double> > > & _gradPseudoValuesAtoms);
      void initNonLocalPseudoPotential();
      void initNonLocalPseudoPotential_OV();
      void computeSparseStructureNonLocalProjectors();
      void computeSparseStructureNonLocalProjectors_OV();
      void computeElementalProjectorKets();


      /**
       *@brief Sets homegeneous dirichlet boundary conditions for total potential constraints on
       * non-periodic boundary (boundary id==0).
       *
       * @param[in] dofHandler
       * @param[out] constraintMatrix ConstraintMatrix object with homogeneous
       * Dirichlet boundary condition entries added
       */
      void applyHomogeneousDirichletBC(const dealii::DoFHandler<3> & _dofHandler,
	                               dealii::ConstraintMatrix & constraintMatrix);

      void computeElementalOVProjectorKets();

      /**
       *@brief Computes total charge by integrating the electron-density
       */
      double totalCharge(const dealii::DoFHandler<3> & dofHandlerOfField,
			 const vectorType & rhoNodalField,
			 std::map<dealii::CellId, std::vector<double> > & rhoQuadValues);


      double totalCharge(const dealii::DoFHandler<3> & dofHandlerOfField,
			 const vectorType & rhoNodalField);


      double totalCharge(const dealii::DoFHandler<3> & dofHandlerOfField,
			 const std::map<dealii::CellId, std::vector<double> > *rhoQuadValues);


      double totalCharge(const dealii::MatrixFree<3,double> & matrixFreeDataObject,
			 const vectorType & rhoNodalField);


      double fieldl2Norm(const dealii::MatrixFree<3,double> & matrixFreeDataObject,
			 const vectorType & rhoNodalField);



      /**
       *@brief Computes net magnetization from the difference of local spin densities
       */
      double totalMagnetization(const std::map<dealii::CellId, std::vector<double> > *rhoQuadValues) ;

      /**
       *@brief normalize the electron density
       */
      void normalizeRho();

      /**
       *@brief Computes output electron-density from wavefunctions
       */
      void compute_rhoOut(const bool isConsiderSpectrumSplitting);

      /**
       *@brief Mixing schemes for mixing electron-density
       */
      double mixing_simple();
      double mixing_anderson();
      double mixing_simple_spinPolarized();
      double mixing_anderson_spinPolarized();
      double mixing_broyden();
      double mixing_broyden_spinPolarized();
      double nodalDensity_mixing_simple(kerkerSolverProblem<C_num1DKerkerPoly<FEOrder>()> & solverProblem,
					dealiiLinearSolver & dealiiLinearSolver);
      double nodalDensity_mixing_anderson(kerkerSolverProblem<C_num1DKerkerPoly<FEOrder>()> & solverProblem,
					  dealiiLinearSolver & dealiiLinearSolver);


      /**
       * Re solves the all electrostatics on a h refined mesh, and computes
       * the corresponding energy. This function
       * is called after reaching the ground state electron density. Currently the h refinement
       * is hardcoded to a one subdivison of carser mesh
       * FIXME: The function is not yet extened to the case when point group symmetry is used.
       * However, it works for time reversal symmetry.
       *
       */
      void computeElectrostaticEnergyHRefined(const bool computeForces=true);
      void computeElectrostaticEnergyPRefined();
      /**
       *@brief Computes Fermi-energy obtained by imposing constraint on the number of electrons
       */
      void compute_fermienergy(const std::vector<std::vector<double>> & eigenValuesInput,
	                       const double numElectronsInput);
      /**
       *@brief Computes Fermi-energy obtained by imposing separate constraints on the number of spin-up and spin-down electrons
       */
      void compute_fermienergy_constraintMagnetization(const std::vector<std::vector<double>> & eigenValuesInput);

      /**
       *@brief compute density of states and local density of states
       */
      void compute_tdos(const std::vector<std::vector<double> > & eigenValuesInput,
			const std::string & fileName);

      void compute_ldos(const std::vector<std::vector<double> > & eigenValuesInput,
			const std::string & fileName);

      void compute_pdos(const std::vector<std::vector<double>> & eigenValuesInput,
                        const std::string & fileName);


      /**
       *@brief compute localization length
       */
      void compute_localizationLength(const std::string & locLengthFileName);

      /**
       *@brief write wavefunction solution fields
       */
      void outputWfc();

      /**
       *@brief write electron density solution fields
       */
      void outputDensity();

      /**
       *@brief write the KS eigen values for given BZ sampling/path
       */
      void writeBands();

      /**
       *@brief Computes the volume of the domain
       */
      double computeVolume(const dealii::DoFHandler<3> & _dofHandler);

      /**
       *@brief Deforms the domain by the given deformation gradient and reinitializes the
       * dftClass datastructures.
       */
      void deformDomain(const Tensor<2,3,double> & deformationGradient);

      /**
       *@brief Computes inner Product and Y = alpha*X + Y for complex vectors used during
       * periodic boundary conditions
       */

#ifdef USE_COMPLEX
      std::complex<double> innerProduct(vectorType & a,
					vectorType & b);

      void alphaTimesXPlusY(std::complex<double>   alpha,
			    vectorType           & x,
			    vectorType           & y);

#endif
      /**
       *@brief Sets dirichlet boundary conditions for total potential constraints on
       * non-periodic boundary (boundary id==0). Currently setting homogeneous bc
       *
       */
      void applyPeriodicBCHigherOrderNodes();

      /// objects for various exchange-correlations (from libxc package)
      xc_func_type funcX, funcC;

      /**
       * stores required data for Kohn-Sham problem
       */
      unsigned int numElectrons, numElectronsUp, numElectronsDown, numLevels;
      std::set<unsigned int> atomTypes;
      std::vector<std::vector<double> > atomLocations,atomLocationsFractional,d_reciprocalLatticeVectors, d_domainBoundingVectors,d_atomLocationsInitial;
      std::vector<std::vector<double> > d_atomLocationsAutoMesh;
      std::vector<std::vector<double> > d_imagePositionsAutoMesh;

      /// Gaussian displacements of atoms read from file
      std::vector<Tensor<1,3,double> > d_atomsDisplacementsGaussianRead;

      bool d_isAtomsGaussianDisplacementsReadFromFile=false;

      /// Gaussian generator parameter for force computation and Gaussian deformation of atoms and FEM mesh
      /// Gaussian generator: Gamma(r)= exp(-(r/d_gaussianConstant)^2)
      const double d_gaussianConstantForce=0.75;

      /// vector of lendth number of periodic image charges with corresponding master chargeIds
      std::vector<int> d_imageIds;
      std::vector<int> d_imageIdsAutoMesh;


      /// vector of length number of periodic image charges with corresponding charge values
      std::vector<double> d_imageCharges;

      /// vector of length number of periodic image charges with corresponding
      /// positions in cartesian coordinates
      std::vector<std::vector<double> > d_imagePositions;

      /// globalChargeId to ImageChargeId Map
      std::vector<std::vector<int> > d_globalChargeIdToImageIdMap;

      /// vector of lendth number of periodic image charges with corresponding master chargeIds
      /// , generated with a truncated pspCutoff
      std::vector<int> d_imageIdsTrunc;

      /// vector of length number of periodic image charges with corresponding charge values
      /// , generated with a truncated pspCutoff
      std::vector<double> d_imageChargesTrunc;

      /// vector of length number of periodic image charges with corresponding
      /// positions in cartesian coordinates, generated with a truncated pspCutOff
      std::vector<std::vector<double> > d_imagePositionsTrunc;

      /// globalChargeId to ImageChargeId Map generated with a truncated pspCutOff
      std::vector<std::vector<int> > d_globalChargeIdToImageIdMapTrunc;

      /// distance from the domain till which periodic images will be considered
      const double d_pspCutOff=40.0;

      /// distance from the domain till which periodic images will be considered
      const double d_pspCutOffTrunc=8.0;

      std::vector<orbital> waveFunctionsVector;
      std::map<unsigned int, std::map<unsigned int, std::map<unsigned int, alglib::spline1dinterpolant*> > > radValues;
      std::map<unsigned int, std::map<unsigned int, std::map <unsigned int, double> > >outerValues;

      /**
       * meshGenerator based object
       */
      triangulationManager d_mesh;

      double d_autoMeshMaxJacobianRatio;
      unsigned int d_autoMesh;


      /// affine transformation object
      meshMovementAffineTransform d_affineTransformMesh;

      /// meshMovementGaussianClass object
      meshMovementGaussianClass d_gaussianMovePar;

      std::vector<Tensor<1,3,double>> d_gaussianMovementAtomsNetDisplacements;
      std::vector<Point<C_DIM> > d_controlPointLocationsCurrentMove;
      double d_gaussianConstantAutoMove;

      /// volume of the domain
      double d_domainVolume;

      /// init wfc trunctation radius
      double d_wfcInitTruncation;

      /**
       * dealii based FE data structres
       */
      FESystem<3>        FE, FEEigen;
      DoFHandler<3>      dofHandler, dofHandlerEigen, d_dofHandlerPRefined;
      unsigned int       eigenDofHandlerIndex,phiExtDofHandlerIndex,phiTotDofHandlerIndex,forceDofHandlerIndex;
      unsigned int       densityDofHandlerIndex;
      MatrixFree<3,double> matrix_free_data, d_matrixFreeDataPRefined;
      std::map<types::global_dof_index, Point<3> > d_supportPoints, d_supportPointsEigen;
      std::vector<const ConstraintMatrix * > d_constraintsVector;

      /**
       * parallel objects
       */
      const MPI_Comm   mpi_communicator;
      const MPI_Comm   interpoolcomm;
      const MPI_Comm   interBandGroupComm;
      const unsigned int n_mpi_processes;
      const unsigned int this_mpi_process;
      IndexSet   locally_owned_dofs, locally_owned_dofsEigen;
      IndexSet   locally_relevant_dofs, locally_relevant_dofsEigen;
      std::vector<dealii::types::global_dof_index> local_dof_indicesReal, local_dof_indicesImag;
      std::vector<dealii::types::global_dof_index> localProc_dof_indicesReal,localProc_dof_indicesImag;
      std::vector<bool> selectedDofsHanging;

      forceClass<FEOrder> * forcePtr;
      symmetryClass<FEOrder> * symmetryPtr;
      geoOptIon<FEOrder> * geoOptIonPtr;
      geoOptCell<FEOrder> * geoOptCellPtr;

      /**
       * constraint Matrices
       */

      /**
       *object which is used to store dealii constraint matrix information
       *using STL vectors. The relevant dealii constraint matrix
       *has hanging node constraints and periodic constraints(for periodic problems)
       *used in eigen solve
       */
      dftUtils::constraintMatrixInfo constraintsNoneEigenDataInfo;

      /**
       *object which is used to store dealii constraint matrix information
       *using STL vectors. The relevant dealii constraint matrix
       *has hanging node constraints and periodic constraints(for periodic problems)
       *used in eigen solve
       */
      dftUtils::constraintMatrixInfo constraintsNoneEigenDataInfoPrev;

      /**
       *object which is used to store dealii constraint matrix information
       *using STL vectors. The relevant dealii constraint matrix
       *has hanging node constraints used in Poisson problem solution
       *
       */
      dftUtils::constraintMatrixInfo constraintsNoneDataInfo;

      /**
       *object which is used to store dealii constraint matrix information
       *using STL vectors. The relevant dealii constraint matrix
       *has hanging node constraints used in Poisson problem solution
       *
       */
      dftUtils::constraintMatrixInfo constraintsNoneDataInfo2;


      ConstraintMatrix constraintsNone, constraintsNoneEigen, d_constraintsForTotalPotential, d_noConstraints, d_constraintsPRefined;


      /**
       * data storage for Kohn-Sham wavefunctions
       */
      std::vector<std::vector<double> > eigenValues;

      /// Spectrum split higher eigenvalues computed in Rayleigh-Ritz step
      std::vector<std::vector<double> > eigenValuesRRSplit;
      std::vector<dealii::parallel::distributed::Vector<dataTypes::number> > d_eigenVectorsFlattened;
      std::vector<std::vector<dataTypes::number> > d_eigenVectorsFlattenedSTL;
      std::vector<std::vector<dataTypes::number> > d_eigenVectorsRotFracDensityFlattenedSTL;

      /// parallel message stream
      ConditionalOStream  pcout;

      /// compute-time logger
      TimerOutput computing_timer;
      TimerOutput computingTimerStandard;

      /// A plain global timer to track only the total elapsed time after every ground-state solve
      dealii::Timer d_globalTimer;

      //dft related objects
      std::map<dealii::CellId, std::vector<double> > *rhoInValues, *rhoOutValues, *rhoInValuesSpinPolarized, *rhoOutValuesSpinPolarized;
      std::deque<std::map<dealii::CellId,std::vector<double> >> rhoInVals, rhoOutVals, rhoInValsSpinPolarized, rhoOutValsSpinPolarized;

      vectorType d_rhoInNodalValues, d_rhoOutNodalValues, d_preCondResidualVector;
      std::deque<vectorType> d_rhoInNodalVals, d_rhoOutNodalVals;


      std::map<dealii::CellId, std::vector<double> > * gradRhoInValues, *gradRhoInValuesSpinPolarized;
      std::map<dealii::CellId, std::vector<double> > * gradRhoOutValues, *gradRhoOutValuesSpinPolarized;
      std::deque<std::map<dealii::CellId,std::vector<double> >> gradRhoInVals,gradRhoInValsSpinPolarized,gradRhoOutVals, gradRhoOutValsSpinPolarized;

      // Broyden mixing related objects
      std::map<dealii::CellId, std::vector<double> > FBroyden, gradFBroyden ;
      std::deque<std::map<dealii::CellId,std::vector<double> >> dFBroyden, graddFBroyden ;
      std::deque<std::map<dealii::CellId,std::vector<double> >> uBroyden, gradUBroyden ;
      std::deque<double>  wtBroyden;
      double w0Broyden = 0.0 ;
      //


      // storage for total electrostatic potential solution vector corresponding to input scf electron density
      vectorType d_phiTotRhoIn;

      // storage for total electrostatic potential solution vector corresponding to output scf electron density
      vectorType d_phiTotRhoOut;

      // storage for sum of nuclear electrostatic potential
      vectorType d_phiExt;

      // storage for projection of rho cell quadrature data to nodal field
      vectorType d_rhoNodalField;

      // storage for projection of rho cell quadrature data to nodal field
      vectorType d_rhoNodalFieldSpin0;

      // storage for projection of rho cell quadrature data to nodal field
      vectorType d_rhoNodalFieldSpin1;

      double d_pspTail = 8.0;
      std::map<dealii::CellId, std::vector<double> > d_pseudoVLoc;

      /// Internal data:: map for cell id to gradient of Vpseudo local of individual atoms. Only for atoms
      /// whose psp tail intersects the local domain.
      std::map<unsigned int,std::map<dealii::CellId, std::vector<double> > > d_gradPseudoVLocAtoms;


      /// Internal data: map for cell id to sum Vpseudo local of all atoms whose psp tail intersects the local domain.
      std::map<dealii::CellId, std::vector<double> > d_gradPseudoVLoc;

      std::vector<std::vector<double> > d_localVselfs;

      //nonlocal pseudopotential related objects used only for pseudopotential calculation

      //
      // Store the map between the "pseudo" wave function Id and the function Id details (i.e., global splineId, l quantum number, m quantum number)
      //
      std::vector<std::vector<int> > d_pseudoWaveFunctionIdToFunctionIdDetails;

      //
      // Store the map between the "pseudo" potential Id and the function Id details (i.e., global splineId, l quantum number)
      //
      std::vector<std::vector<int> > d_deltaVlIdToFunctionIdDetails;

      //
      // vector to store the number of pseudowave functions/pseudo potentials associated with an atom
      //
      std::vector<int> d_numberPseudoAtomicWaveFunctions;
      std::vector<int> d_numberPseudoPotentials;
      std::vector<int> d_nonLocalAtomGlobalChargeIds;

      //
      //matrices denoting the sparsity of nonlocal projectors and elemental projector matrices
      //
      std::vector<std::vector<int> > d_sparsityPattern;
      std::vector<std::vector<DoFHandler<3>::active_cell_iterator> > d_elementIteratorsInAtomCompactSupport;
      std::vector<std::vector<unsigned int> > d_elementIdsInAtomCompactSupport;
      std::vector<std::vector<DoFHandler<3>::active_cell_iterator> > d_elementOneFieldIteratorsInAtomCompactSupport;
      std::vector<std::vector<int> > d_nonLocalAtomIdsInElement;
      std::vector<unsigned int> d_nonLocalAtomIdsInCurrentProcess;
      IndexSet d_locallyOwnedProjectorIdsCurrentProcess;
      IndexSet d_ghostProjectorIdsCurrentProcess;
      std::map<std::pair<unsigned int,unsigned int>, unsigned int> d_projectorIdsNumberingMapCurrentProcess;
#ifdef USE_COMPLEX
      std::vector<std::vector<std::vector<std::vector<std::complex<double> > > > > d_nonLocalProjectorElementMatrices,d_nonLocalProjectorElementMatricesConjugate,d_nonLocalProjectorElementMatricesTranspose;


      std::vector<dealii::parallel::distributed::Vector<std::complex<double> > > d_projectorKetTimesVectorPar;

      /// parallel vector used in nonLocalHamiltionian times wavefunction vector computation
      /// pre-initialization of the parallel layout is more efficient than creating the parallel
      /// layout for every nonLocalHamiltionan times wavefunction computation
      dealii::parallel::distributed::Vector<std::complex<double> >  d_projectorKetTimesVectorParFlattened;
#else
      std::vector<std::vector<std::vector<double> > > d_nonLocalProjectorElementMatrices,d_nonLocalProjectorElementMatricesConjugate,d_nonLocalProjectorElementMatricesTranspose;


      std::vector<dealii::parallel::distributed::Vector<double> > d_projectorKetTimesVectorPar;

      /// parallel vector used in nonLocalHamiltionian times wavefunction vector computation
      /// pre-initialization of the parallel layout is more efficient than creating the parallel
      /// layout for every nonLocalHamiltionan times wavefunction computation
      dealii::parallel::distributed::Vector<double> d_projectorKetTimesVectorParFlattened;
#endif

      //
      //storage for nonlocal pseudopotential constants
      //
      std::vector<std::vector<double> > d_nonLocalPseudoPotentialConstants;

      //
      // spline vector for data corresponding to each spline of pseudo wavefunctions
      //
      std::vector<alglib::spline1dinterpolant> d_pseudoWaveFunctionSplines;

      //
      // spline vector for data corresponding to each spline of delta Vl
      //
      std::vector<alglib::spline1dinterpolant> d_deltaVlSplines;

      //
      //vector of outermost Points for various radial Data
      //
      std::vector<double> d_outerMostPointPseudoWaveFunctionsData;
      std::vector<double> d_outerMostPointPseudoPotData;
      std::vector<double> d_outerMostPointPseudoProjectorData;

      /// map of atom node number and atomic weight
      std::map<dealii::types::global_dof_index, double> d_atomNodeIdToChargeMap;

      /// vselfBinsManager object
      vselfBinsManager<FEOrder> d_vselfBinsManager;

      /// kPoint cartesian coordinates
      std::vector<double> d_kPointCoordinates;

      /// k point crystal coordinates
      std::vector<double> kPointReducedCoordinates;

      /// k point weights
      std::vector<double> d_kPointWeights;

      /// closest tria vertex
      std::vector<Point<3>> d_closestTriaVertexToAtomsLocation;
      std::vector<Tensor<1,3,double> > d_dispClosestTriaVerticesToAtoms;

      /// global k index of lower bound of the local k point set
      unsigned int lowerBoundKindex ;
      /**
       * Recomputes the k point cartesian coordinates from the crystal k point coordinates
       * and the current lattice vectors, which can change in each ground state solve when
       * isCellOpt is true
       */
      void recomputeKPointCoordinates();

      /// fermi energy
      double fermiEnergy, fermiEnergyUp, fermiEnergyDown, d_groundStateEnergy, d_groundStateEnergyInitial;

      //chebyshev filter variables and functions
      //int numPass ; // number of filter passes

      std::vector<double> a0;
      std::vector<double> bLow;


      vectorType d_tempEigenVec;
      vectorType d_tempEigenVecPrev;

      /**
      * @ nscf variables
      */
     bool scfConverged;
     void nscf(kohnShamDFTOperatorClass<FEOrder> & kohnShamDFTEigenOperator,
	      chebyshevOrthogonalizedSubspaceIterationSolver & subspaceIterationSolver);
     void initnscf(kohnShamDFTOperatorClass<FEOrder> & kohnShamDFTEigenOperator,poissonSolverProblem<FEOrder> & phiTotalSolverProblem,
	      dealiiLinearSolver & dealiiCGSolver) ;

      /**
       * @brief compute the maximum of the residual norm of the highest occupied state among all k points
       */
      double computeMaximumHighestOccupiedStateResidualNorm(const std::vector<std::vector<double> > & residualNormWaveFunctionsAllkPoints,
							    const std::vector<std::vector<double> > & eigenValuesAllkPoints,
							    const double _fermiEnergy);


      void kohnShamEigenSpaceCompute(const unsigned int s,
				     const unsigned int kPointIndex,
				     kohnShamDFTOperatorClass<FEOrder> & kohnShamDFTEigenOperator,
				     chebyshevOrthogonalizedSubspaceIterationSolver & subspaceIterationSolver,
				     std::vector<double> & residualNormWaveFunctions,
				     const bool isSpectrumSplit=false,
				     const bool useMixedPrec=false,
                                     const bool isFirstScf=false,
				     const bool useFullMassMatrixGEP=false);

     void kohnShamEigenSpaceComputeNSCF(const unsigned int spinType,
				    const unsigned int kPointIndex,
				    kohnShamDFTOperatorClass<FEOrder> & kohnShamDFTEigenOperator,
				    chebyshevOrthogonalizedSubspaceIterationSolver & subspaceIterationSolver,
				    std::vector<double>                            & residualNormWaveFunctions,
				    unsigned int ipass) ;

      void computeResidualNorm(const std::vector<double> & eigenValuesTemp,
			       kohnShamDFTOperatorClass<FEOrder> & kohnShamDFTEigenOperator,
			       std::vector<vectorType> & X,
			       std::vector<double> & residualNorm) const;

    };

}

#endif
