#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>

#define PRINT		0	/* enable/disable prints. */
#define SIZE    100ULL

#if PRINT
#define pr(...)		do { fprintf(stderr, __VA_ARGS__); } while (0)
#else
#define pr(...)		/* no effect at all */
#endif

#define MIN(a,b)	(((a)<=(b))?(a):(b))

typedef struct graph_t	graph_t;
typedef struct node_t	node_t;
typedef struct edge_t	edge_t;
typedef struct list_t	list_t;
typedef struct myargs myargs;
typedef struct instruct instruct;
typedef struct nodelist nodelist;


struct list_t {
	edge_t*		edge;
	list_t*		next;
};

struct node_t {
	int		h;	/* height.			*/
	int		e;	/* excess flow.			*/
	list_t*		edge;	/* adjacency list.		*/
	node_t*		next;	/* with excess preflow.		*/
	int futureExcess;
	int active;
  //pthread_mutex_t mutex;  /* node mutex lock */
};

struct edge_t {
	node_t*		u;	/* one of the two nodes.	*/
	node_t*		v;	/* the other. 			*/
	int		f;	/* flow > 0 if from u to v.	*/
	int		c;	/* capacity.			*/
};

struct graph_t {
	int		n;	/* nodes.			*/
	int		m;	/* edges.			*/
	node_t*		v;	/* array of n nodes.		*/
	edge_t*		e;	/* array of m edges.		*/
	node_t*		s;	/* source.			*/
	node_t*		t;	/* sink.			*/
	node_t*		excess;	/* nodes with e > 0 except s,t.	*/
  //pthread_mutex_t mutex;  /* graph mutex lock */
  int working;

};

struct instruct{
  int isPush;
  node_t* u;
  node_t* v;
  edge_t* edge;
  int flow;
};

struct myargs{
  graph_t* g;
  node_t* nodes[SIZE];
  int count;
  int nbrNodes;
  //nodelist* nodes;
  instruct instructions[SIZE];
  pthread_barrier_t* barrier;
  // add more things with barriers and threads?
};

/*struct nodelist{
  node_t* node;
  nodelist* next;
};*/

static char* progname;

#if PRINT

static int id(graph_t* g, node_t* v)
{

	return v - g->v;
}
#endif

void error(const char* fmt, ...)
{
	va_list		ap;
	char		buf[BUFSIZ];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);

	if (progname != NULL)
		fprintf(stderr, "%s: ", progname);

	fprintf(stderr, "error: %s\n", buf);
	exit(1);
}

static int next_int()
{
        int     x;
        int     c;

	      x = 0;
        while (isdigit(c = getchar()))
                x = 10 * x + c - '0';

        return x;
}

static void* xmalloc(size_t s)
{
	void*		p;

	p = malloc(s);

	if (p == NULL)
		error("out of memory: malloc(%zu) failed", s);

	return p;
}

static void* xcalloc(size_t n, size_t s)
{
	void*		p;

	p = xmalloc(n * s);

	memset(p, 0, n * s);


	return p;
}

static void add_edge(node_t* u, edge_t* e)
{
	list_t*		p;

	p = xmalloc(sizeof(list_t));
	p->edge = e;
	p->next = u->edge;
	u->edge = p;
}

static void connect(node_t* u, node_t* v, int c, edge_t* e)
{

	e->u = u;
	e->v = v;
	e->c = c;

	add_edge(u, e);
	add_edge(v, e);
}

static graph_t* new_graph(FILE* in, int n, int m)
{
	graph_t*	g;
	node_t*		u;
	node_t*		v;
	int		i;
	int		a;
	int		b;
	int		c;

	g = xmalloc(sizeof(graph_t));

	g->n = n;
	g->m = m;

  g->working = 0;

	g->v = xcalloc(n, sizeof(node_t));
	g->e = xcalloc(m, sizeof(edge_t));

	g->s = &g->v[0];
	g->t = &g->v[n-1];
	g->excess = NULL;

	for (i = 0; i < m; i += 1) {
		a = next_int();
		b = next_int();
		c = next_int();
		u = &g->v[a];
		v = &g->v[b];
		connect(u, v, c, g->e+i);
	}

	return g;
}

static void enter_excess(graph_t* g, node_t* v)
{

	if (v->active != 1 && v != g->t && v != g->s) {
		v->next = g->excess;
		g->excess = v;
		v->active = 1;
	}

}

static node_t* leave_excess(graph_t* g)
{
	node_t*		v;

  v = g->excess;
	if (v != NULL){
		g->excess = v->next;
		v->active = 0;
  }

	return v;
}

static void push(graph_t* g, node_t* u, node_t* v, edge_t* e, int d)
{

	//pr("push from %d to %d: ", id(g, u), id(g, v));
	//pr("f = %d, c = %d, so ", e->f, e->c);
	//pr("pushing %d\n", d);
	__transaction_atomic{
		u->e -= d;
		v->e += d;
	}

	assert(d >= 0);
	assert(u->e >= 0);
	assert(abs(e->f) <= e->c);

	if (u->e > 0) {
		enter_excess(g, u);
	}

	if (v->e == d) {
		enter_excess(g, v);
	}
}

static void relabel(graph_t* g, node_t* u)
{

	u->h += 1;

	//pr("relabel %d now h = %d\n", id(g, u), u->h);

	enter_excess(g, u);
}

static node_t* other(node_t* u, edge_t* e)
{
	if (u == e->u)
		return e->v;
	else
		return e->u;
}

static void* task_1(void* arg){

  myargs *args = arg;

  graph_t* g = args->g;
	node_t*		s;
	node_t*		u;
	node_t*		v;
	edge_t*		e;
	list_t*		p;
  int   d;
	int		b;
  instruct* current_instruction;

    // Should wait on the barrier here?
  pthread_barrier_wait(args->barrier);

	/* then loop until only s and/or t have excess preflow. */
  work:

	  while (args->count > 0) {

      u = args->nodes[args->count-1];
      args->nodes[args->count] = NULL;

	  	v = NULL;
	  	p = u->edge;

	  	while (p != NULL) {
	  		e = p->edge;
	  		p = p->next;

	  		if (u == e->u) {
	  			v = e->v;
	  			b = 1;
	  		} else {
	  			v = e->u;
	  			b = -1;
	  		}

	  		if (u->h > v->h && b * e->f < e->c){
	  			break;
        }
	  		else{
	  			v = NULL;
        }
	  	}

	  	if (v != NULL){
	  		//push instruct
        if (u == e->u) {
      		d = MIN(u->e, e->c - e->f);
					__transaction_atomic {
						e->f += d;
					}
      	} else {
      		d = MIN(u->e, e->c + e->f);
					__transaction_atomic {
						e->f -= d;
					}
      	}

        current_instruction = &(args->instructions[args->count-1]);
        current_instruction->u = u;
        current_instruction->v = v;
        current_instruction->edge = e;
        current_instruction->isPush = 1;
        current_instruction->flow = d;

				__transaction_atomic{
					u->futureExcess -= d;
					v->futureExcess += d;
				}

				//push(g,u,v,e,d);

	  	} else{
	  		//relabel
        current_instruction = &(args->instructions[args->count-1]);
        current_instruction->u = u;
        current_instruction->isPush = 0;
      }

      args->count -= 1;
	  }

  pthread_barrier_wait(args->barrier);
  pthread_barrier_wait(args->barrier);

  if(g->working == 1){
    goto work;
  }

  //fprintf(stderr, "Thread done\n");
  return 0;
}

static void* task_2(graph_t* g, myargs* arg, int number_of_threads){
  instruct* curr_instruct;

  for (int i = 0; i < number_of_threads; i++) {

    for (int j = 0; j < arg[i].nbrNodes; j++) {
      curr_instruct = &(arg[i].instructions[j]);

      if (curr_instruct->isPush == 1) {

				if(curr_instruct->u->futureExcess != 0){
					curr_instruct->u->e += curr_instruct->u->futureExcess;
					curr_instruct->u->futureExcess = 0;
					enter_excess(g,curr_instruct->u);
				}
				if(curr_instruct->v->futureExcess != 0){
					curr_instruct->v->e += curr_instruct->v->futureExcess;
					curr_instruct->v->futureExcess = 0;
					enter_excess(g,curr_instruct->v);
				}
      }
      else {

        relabel(g,curr_instruct->u);

      }
    }

  }
}

static void giveNodes(graph_t* g, myargs* arg, int number_of_threads)
{
  node_t* u;
  int i = 0;

  for (int i = 0; i < number_of_threads; i++){
    arg[i].nbrNodes = 0;
  }

  while ((u = leave_excess(g)) != NULL){
    arg[i].nodes[arg[i].count] = u;
    arg[i].count += 1;
    arg[i].nbrNodes += 1;

    if(i < number_of_threads-1){
      i++;
    } else{
      i = 0;
    }
  }
}

static int preflow(graph_t* g, int number_of_threads)
{
	node_t*		s;
	node_t*		u;
	node_t*		v;
	edge_t*		e;
	list_t*		p;
  pthread_t threads[number_of_threads];
	int		b;

	s = g->s;
	s->h = g->n;

	p = s->edge;

  g->working = 1;

	while (p != NULL) {
		e = p->edge;
		p = p->next;

		s->e += e->c;
		push(g, s, other(s, e), e, e->c);
	}

  // create barrier here and start it?
  pthread_barrier_t barrier;
  pthread_barrier_init(&barrier,NULL,number_of_threads+1);

  myargs arg[number_of_threads];
  for (int i = 0; i < number_of_threads; i++) {
    arg[i].g = g;
    arg[i].barrier = &barrier;
    arg[i].count = 0;
    arg[i].nbrNodes = 0;
  }

  giveNodes(g,arg, number_of_threads);

  for (int i = 0; i < number_of_threads; i += 1){
    pthread_create(&threads[i], NULL, task_1, &arg[i]);
  }

  pthread_barrier_wait(&barrier);
  pthread_barrier_wait(&barrier);

  while(1) {
    task_2(g,arg,number_of_threads);
    if(g->excess == NULL){
      break;
    }
    giveNodes(g,arg,number_of_threads);
    pthread_barrier_wait(&barrier);
    pthread_barrier_wait(&barrier);
  }

  g->working = 0;
  pthread_barrier_wait(&barrier);

  for (int i = 0; i < number_of_threads; i += 1){
    pthread_join(threads[i], NULL);
  }

	return g->t->e;
}


static void free_graph(graph_t* g)
{
	int		i;
	list_t*		p;
	list_t*		q;

	for (i = 0; i < g->n; i += 1) {
		p = g->v[i].edge;
		while (p != NULL) {
			q = p->next;
			free(p);
			p = q;
		}
	}
	free(g->v);
	free(g->e);
	free(g);
}

int main(int argc, char* argv[])
{

  pr("les go\n");
	FILE*		in;	/* input file set to stdin	*/
	graph_t*	g;	/* undirected graph. 		*/
	int		f;	/* output from preflow.		*/
	int		n;	/* number of nodes.		*/
	int		m;	/* number of edges.		*/

	progname = argv[0];	/* name is a string in argv[0]. */

	in = stdin;		/* same as System.in in Java.	*/

	n = next_int();
	m = next_int();

	/* skip C and P from the 6railwayplanning lab in EDAF05 */
	next_int();
	next_int();

	g = new_graph(in, n, m);

	fclose(in);

  int number_of_threads = 12;
	f = preflow(g, number_of_threads);

	printf("f = %d\n", f);

	free_graph(g);

	return 0;
}
