#include "GraphMatRuntime.cpp"

#include "Degree.cpp"

class dPR {
  public:
    double delta;
    double pagerank;
    int degree;
  public:
    dPR() {
      delta = 0.3;
      pagerank = 0.3;
      degree = 0;
    }
    int operator!=(const dPR& p) {
      return (fabs(p.pagerank-this->pagerank)>1e-8);
    }
    void print() {
      printf("current pr = %.4lf \n", pagerank);
    }
};


class DeltaPageRank : public GraphProgram<double, double, dPR> {
  public:
    double alpha;
    int iter;

  public:

  DeltaPageRank(double a=0.3) {
    alpha = a;
    iter = 0;
    this->order = OUT_EDGES;
    this->activity=ACTIVE_ONLY;
  }

  void reduce_function(double& a, const double& b) const {
    a += b;
  }
  void process_message(const double& message, const int edge_val, const dPR& vertexprop, double& res) const {
    res = message;
  }
  bool send_message(const dPR& vertexprop, double& message) const {
    if (vertexprop.degree == 0) {
      message = 0.0;
    } else {
      message = vertexprop.delta/(double)vertexprop.degree;
    }

    return true;
  }

  void apply(const double& message_out, dPR& vertexprop) {
    if (fabs(vertexprop.delta) > 1e-8) vertexprop.delta = 0.0;
    vertexprop.delta += (1.0-alpha)*message_out;
    if (fabs(vertexprop.delta) > 1e-8) {
      vertexprop.pagerank += vertexprop.delta;
    }
  }

  void do_every_iteration(int iteration_number) {
    iter++;
  }

};

//-------------------------------------------------------------------------


void run_pagerank(const char* filename, int nthreads) {

  Graph<dPR> G;
  DeltaPageRank dpr;
  Degree<dPR> dg;
  
  G.ReadMTX(filename, nthreads*4); //nthread*4 pieces of matrix

  auto dg_tmp = graph_program_init(dg, G);

  struct timeval start, end;
  gettimeofday(&start, 0);

  G.setAllActive();
  run_graph_program(&dg, G, 1, &dg_tmp);

  gettimeofday(&end, 0);
  double time = (end.tv_sec-start.tv_sec)*1e3+(end.tv_usec-start.tv_usec)*1e-3;
  printf("Degree Time = %.3f ms \n", time);

  graph_program_clear(dg_tmp);
  
  auto dpr_tmp = graph_program_init(dpr, G);

  gettimeofday(&start, 0);

  G.setAllActive();
  run_graph_program(&dpr, G, -1, &dpr_tmp);
  
  gettimeofday(&end, 0);
  time = (end.tv_sec-start.tv_sec)*1e3+(end.tv_usec-start.tv_usec)*1e-3;
  printf("PR Time = %.3f ms \n", time);

  graph_program_clear(dpr_tmp);

  for (int i = 0; i < std::min((unsigned long long int)25, (unsigned long long int)G.getNumberOfVertices()); i++) { 
    printf("%d : %d %f\n", i, G.getVertexproperty(i).degree, G.getVertexproperty(i).pagerank);
  }
}

int main(int argc, char* argv[]) {

  const char* input_filename = argv[1];

  if (argc < 2) {
    printf("Correct format: %s A.mtx\n", argv[0]);
    return 0;
  }

#pragma omp parallel
  {
#pragma omp single
    {
      nthreads = omp_get_num_threads();
      printf("num threads got: %d\n", nthreads);
    }
  }
  

  run_pagerank(input_filename, nthreads);
  
}

