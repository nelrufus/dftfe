#ifndef poisson_H_
#define poisson_H_
#include "headers.h"
#include "dft.h"

//Initialize Namespace
using namespace dealii;

typedef parallel::distributed::Vector<double> vectorType;

//Define poisson class
template <int dim>
class poisson
{
  friend class dft; 
public:
  poisson(DoFHandler<dim>* _dofHandler);

  //operator operations
  void vmult (vectorType &dst,
	      const vectorType &src) const;
  
  //solve interface
  void solve(vectorType& solution, 
	     vectorType& residual,
	     ConstraintMatrix& constraints,
	     std::map<unsigned int, double>& atoms,
	     Table<2,double>* rhoValues=0
	     );

private:
  void apply_dirichlet_bcs();
  void setup_matrixfree ();
  void updateRHS();
  void computeRHS(const MatrixFree<dim,double> &data,
		  vectorType &dst,
		  const vectorType &src,
		  const std::pair<unsigned int,unsigned int> &cell_range) const;
  bool updateRHSValue;
    
  //FE data structres
  FE_Q<dim>          FE;
  DoFHandler<dim>*    dofHandler;

  //matrix free structure
  MatrixFree<dim,number> data;

  //parallel objects
  MPI_Comm mpi_communicator;
  const unsigned int n_mpi_processes;
  const unsigned int this_mpi_process;
  ConditionalOStream   pcout;
  IndexSet   locally_owned_dofs;
  IndexSet   locally_relevant_dofs;

  //compute-time logger
  TimerOutput computing_timer;
};

#endif
