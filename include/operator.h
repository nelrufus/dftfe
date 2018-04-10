//
// -------------------------------------------------------------------------------------
//
// Copyright (c) 2017 The Regents of the University of Michigan and DFT-FE authors.
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
// --------------------------------------------------------------------------------------
//
// @author Phani Motamarri (2018)
//
#ifndef operatorClass_h
#define operatorClass_h

#include <vector>

#include "headers.h"

typedef dealii::parallel::distributed::Vector<double> vectorType;
class operatorClass {

  //
  // types
  //
  public:
  //
  // methods
  //
  public:

  /**
   * @brief Destructor.
   */
  virtual ~operatorClass() = 0;


  /**
   * @brief initialize operatorClass
   *
   */
  virtual void init() = 0;


  /**
   * @brief compute M matrix
   *
   * @return diagonal M matrix
   */
  virtual void computeMassVector() = 0;


  /**
   * @brief Compute operator times vector or operator times bunch of vectors
   *
   * @param X Vector of Vectors containing current values of X
   * @param Y Vector of Vectors containing operator times vectors product
   */
  virtual void HX(std::vector<vectorType> & x,
		  std::vector<vectorType> & y) = 0;


  /**
   * @brief Compute projection of the operator into orthogonal basis
   *
   * @param X given orthogonal basis vectors
   * @return ProjMatrix projected small matrix 
   */
#ifdef ENABLE_PERIODIC_BC
  virtual void XtHX(std::vector<vectorType> & X,
		    std::vector<std::complex<double> > & ProjHam) = 0;
#else
  virtual void XtHX(std::vector<vectorType> & X,
		    std::vector<double> & ProjHam) = 0;
#endif


  /**
   * @brief Get local dof indices real
   *
   * @return pointer to local dof indices real
   */
  const std::vector<unsigned int> * getLocalDofIndicesReal();

  /**
   * @brief Get local dof indices imag
   *
   * @return pointer to local dof indices real
   */
  const std::vector<unsigned int> * getLocalDofIndicesImag();

  /**
   * @brief Get local proc dof indices real
   *
   * @return pointer to local proc dof indices real
   */
  const std::vector<unsigned int> * getLocalProcDofIndicesReal();


  /**
   * @brief Get local proc dof indices imag
   *
   * @return pointer to local proc dof indices imag
   */
  const std::vector<unsigned int> * getLocalProcDofIndicesImag();

  /**
   * @brief Get constraint matrix eigen
   *
   * @return pointer to constraint matrix eigen
   */
  const dealii::ConstraintMatrix * getConstraintMatrixEigen();


  /**
   * @brief Get relevant mpi communicator
   *
   * @return mpi communicator
   */
  const MPI_Comm & getMPICommunicator();
  

 protected:
    
  /**
   * @brief default Constructor.
   */
  operatorClass();


  /**
   * @brief Constructor.
   */
  operatorClass(MPI_Comm & mpi_comm_replica,
		const std::vector<unsigned int> & localDofIndicesReal,
		const std::vector<unsigned int> & localDofIndicesImag,
		const std::vector<unsigned int> & localProcDofIndicesReal,
		const std::vector<unsigned int> & localProcDofIndicesImag,
		const dealii::ConstraintMatrix  & constraintMatrixEigen);

 protected:

  //data members
  const std::vector<unsigned int> * d_localDofIndicesReal;
  const std::vector<unsigned int> * d_localDofIndicesImag;
  const std::vector<unsigned int> * d_localProcDofIndicesReal;
  const std::vector<unsigned int> * d_localProcDofIndicesImag;
  const dealii::ConstraintMatrix  * d_constraintMatrixEigen;
  MPI_Comm                          d_mpi_communicator;

};

#endif
