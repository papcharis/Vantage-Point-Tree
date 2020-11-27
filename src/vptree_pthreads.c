/**
 * File: vptree_pthreads.c
 * Description:
 *  This is the parallel implementation of the Vantage Point Tree using POSIX threads(pthreads).
 *  Written for "Parallel and Distributed Systems" class
 *  Faculty of Electrical and Computer Engineering AUTH.
 *
 *  The declarations of the functions defined here are in:
 *  File: vptree.h
 *
 * Authors:
 *  Christoforidis Savvas, AEM: 9147, schristofo@ece.auth.gr
 *  Papadakis Charalampos, AEM:9128, papadakic@ece.auth.gr
 *
 * Date:
 *  October-November 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include "vptree.h"

//used for tree seperation
#define MAX_THREADS 2
//used for distance calculation
#define NOTHREADS 4




typedef struct nodeParameters {
  double * data;
  int * idx;
  int n;
  int * levelThreads;
  int d;
  double *distances;
} nodeParameters;

int nodesMade = 0;


pthread_mutex_t mux;
pthread_attr_t attr;
volatile int activeThreads = 0;



volatile int threadCounter = -1;
pthread_mutex_t mux1;
pthread_attr_t attr;


double  distanceSer(double * point, double * vpoint, int d) {
    double tempDist = 0;
    for (int i = 0; i < d; i++){
      tempDist += pow((point[i] - vpoint[i]),2);
    }
    return sqrt(tempDist);
}


void * distancePar(void *data) {
  nodeParameters *T= (nodeParameters *) data;
  int i,j,start,end,iterations=0 , finalIteration;

  double tempDist=0;
  int localThreadCounter = 0 ;

  pthread_mutex_lock(&mux);
  if (*(T->levelThreads)==-1 || *(T->levelThreads)==NOTHREADS-1){
    *(T->levelThreads)=0;
    localThreadCounter = *(T->levelThreads);
  }
  else if(*(T->levelThreads)<NOTHREADS-1){
    (*(T->levelThreads))++;
    localThreadCounter = *(T->levelThreads);
  }
  pthread_mutex_unlock(&mux);

  if((T->n-1)%NOTHREADS ==0){
    iterations = ((T->n)/NOTHREADS);
    start = localThreadCounter *iterations;
    end = start + iterations;
  //  printf("Thread %d is doing iterations %d to %d \n",localThreadCounter,start,end-1);
  }
  else {
    iterations= round((T->n-1)/NOTHREADS);
    finalIteration = (T->n-1) - (NOTHREADS-1)*iterations ;

    if(localThreadCounter == NOTHREADS-1){
      start = (localThreadCounter)*iterations;
      end = start+finalIteration;
    //  printf("Thread %d is doing iterations %d to %d \n",localThreadCounter,start,end-1);
    }
    else{
      start = (localThreadCounter)*iterations;
      end = start + iterations;
  //   printf("Thread %d is doing iterations %d to %d \n",localThreadCounter,start,end-1);
    }
  }

  for (i=start; i<end; i++){
    for(j=0; j<T->d; j++){
      tempDist= tempDist + (*(T->data + (T->n-1)*T->d + j) - *(T->data + i*T->d + j)) * (*(T->data + (T->n-1)*T->d + j) - *(T->data + i*T->d + j));
    }
    T->distances[i]=sqrt(tempDist);
  //  printf("THE DISTANCES IS :  %lf \n" , localDist->distances[i]);
    tempDist =0;
  }
return NULL;
}


double quickSelect(double *v, int len, int k) {
	#	define SWAP(a, b) { tmp = tArray[a]; tArray[a] = tArray[b]; tArray[b] = tmp; }
	int i, st;
	double tmp;
	double * tArray = (double * ) malloc(len * sizeof(double));
	for(int i=0; i<len; i++){
		tArray[i] = v[i];
	}
	for (st = i = 0; i < len - 1; i++) {
		if (tArray[i] > tArray[len-1]) continue;
		SWAP(i, st);
		st++;
	}
	SWAP(len-1, st);
	return k == st	? tArray[st]
			:st > k	? quickSelect(tArray, st, k)
				: quickSelect(tArray + st, len - st, k - st);
}


void * subTreeRec(void * arg) {

  nodeParameters * x = (nodeParameters *) arg;
  vptree * node = (vptree *)malloc(sizeof(vptree));

  if (x->n == 1){
    node->vp = x->data;
    node->idx=x->idx[0];
    node->md=0;
    node->inner=NULL;
    node->outer=NULL;
    return (void *) node;
  }
  if(x->n == 0)
    return NULL;

  pthread_t th1,th2;
  pthread_t thr[NOTHREADS];
  void * r1, * r2;
  nodeParameters p1,p2;
  int parallel = 0;

  int localCounter = -1;
  x->levelThreads= &localCounter;
  x->distances = (double * )malloc((x->n - 1) * sizeof(double));
  double md;

  int innerNum = 0; // total number of inner points
  int outerNum = 0; // total number of outer points
  int inCounter = 0; // inner array counter
  int outCounter = 0; // outer array counter
  int * idxInner = NULL; // inner indices
  int * idxOuter = NULL; // outer indices
  double * Xinner = NULL; // inner points
  double * Xouter = NULL; // outer points

  // set vp and idx
  node->vp = (double * ) malloc(x->d * sizeof(double));
  for (int j = 0; j < x->d; j++) {
    node->vp[j] = * (x->data+ (x->n-1) * x->d + j);
  }
  node->idx = x->idx[x->n-1];


  pthread_mutex_lock(&mux1);
  nodesMade++;
  pthread_mutex_unlock(&mux1);

  if(x->n < 100000){
    for(int i = 0; i < x->n-1; i++){
      x->distances[i]=distanceSer((x->data + i * x->d),node->vp,x->d);
    }
  }
  else{
    //PTHREADS
    for(int i=0; i<NOTHREADS; i++){
      pthread_create(&thr[i], &attr, distancePar, (void *)x);
    }
    for(int i=0; i<NOTHREADS; i++){
      pthread_join(thr[i],NULL);
    }
  }

  md = quickSelect(x->distances, x->n - 1, (int)((x->n - 2) / 2));
  node->md = md;

  outerNum = (int)((x->n - 1) / 2);
  innerNum = x->n - 1 - outerNum;

  if (innerNum != 0) {
    Xinner = (double * ) malloc(innerNum * x->d * sizeof(double));
    idxInner = (int * )malloc(innerNum*sizeof(int));
  }
  if (outerNum != 0) {
    Xouter = (double * ) malloc(outerNum * x->d * sizeof(double));
    idxOuter = (int * )malloc(outerNum*sizeof(int));
  }

  for (int i = 0; i < x->n - 1; i++) {
    if (x->distances[i] <= node->md) {
      for (int j = 0; j < x->d; j++) {
        *(Xinner + inCounter * x->d + j) = * (x->data + i * x->d + j);
      }
      idxInner[inCounter] = x->idx[i];
      inCounter++;
    }
    else {
      for (int j = 0; j < x->d; j++) {
        *(Xouter + outCounter * x->d + j) = * (x->data + i * x->d + j);
      }
      idxOuter[outCounter]= x->idx[i];
      outCounter++;
    }
  }

  // prints

  if (activeThreads < MAX_THREADS) {
    pthread_mutex_lock (&mux);
    activeThreads += 2;
    pthread_mutex_unlock (&mux);
    p1.data = Xinner;
    p1.idx = idxInner;
    p1.n = innerNum;
    p1.d = x->d;
    pthread_create( &th1, &attr, subTreeRec, (void*)(&p1));
    p2.data = Xouter;
    p2.idx = idxOuter;
    p2.n = outerNum;
    p2.d = x->d;
    pthread_create( &th2, &attr, subTreeRec, (void *)(&p2));
    parallel = 1;
  }
  if (parallel) {
    pthread_join(th1,&r1);
    node->inner = (vptree *) r1;
    pthread_join(th2,&r2);
    node->outer = (vptree *) r2;
    pthread_mutex_lock (&mux);
    activeThreads -= 2;
    pthread_mutex_unlock (&mux);
  }
  else {
      free(x->data);
      free(x->idx);
      free(x->distances);
      p1.data = Xinner;
      p1.n = inCounter;
      p1.d = x->d;
      p1.idx = idxInner;
      node->inner = (vptree *)subTreeRec((void *) &p1);

      p2.data = Xouter;
      p2.n = outCounter;
      p2.d = x->d;
      p2.idx = idxOuter;
      node->outer = (vptree *)subTreeRec((void *) &p2);

  }

  // free(p1);
  // free(p2);
  // free(idxInner);
  // free(idxOuter);
  // free(Xinner);
  // free(Xouter);
  return (void *) node;
}


vptree * buildvp(double *X, int n, int d){
  int * idx = (int *) malloc(n * sizeof(int));
   for(int i=0; i<n; i++){
     idx[i] = i;
   }
   nodeParameters p ;
   p.data = X;
   p.n = n;
   p.d = d;
   p.idx = idx;
   p.distances=NULL;
   p.levelThreads =NULL;

   return (vptree *)subTreeRec(&p);

}

vptree * getInner(vptree * T) {
  return T->inner;
}


vptree * getOuter(vptree * T) {
  return T->outer;
}


double * getVP(vptree * T) {
  return T->vp;
}


double getMD(vptree * T) {
  return T->md;
}


int getIDX(vptree * T) {
  return T->idx;
}
