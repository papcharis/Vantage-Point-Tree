/**
 * File: vptree_cilk.c
 * Description:
 *  This is the parallel implementation of the Vantage Point Tree problem using the CILK library.
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
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include "vptree.h"


double  distanceSer(double * point, double * vpoint, int d) {
    double tempDist = 0;
    for (int i = 0; i < d; i++){
      tempDist += pow((point[i] - vpoint[i]),2);
    }
    return sqrt(tempDist);
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


vptree * subTreeRec(double * X, int * idx, int n, int d) {

  vptree * node = (vptree * ) malloc(sizeof(vptree));

  double * distance = (double * ) calloc(n - 1, sizeof(double));
  double md;

  int innerNum = 0; // total number of inner points
  int outerNum = 0; // total number of outer points
  int inCounter = 0; // inner array counter
  int outCounter = 0; // outer array counter
  int * innerIdx = NULL; // inner indices
  int * outerIdx = NULL; // outer indices
  double * Xinner = NULL; // inner points
  double * Xouter = NULL; // outer points

  if (n == 1){
    node->vp=X;
    node->idx=idx[0];
    node->md=0;
    node->inner=NULL;
    node->outer=NULL;
    return node;
  }
  if(n == 0)
    return NULL;

  // set vp and idx
  node->vp = (double * ) malloc(d * sizeof(double));
  for (int j = 0; j < d; j++) {
    node->vp[j] = * (X + (n-1) * d + j);
  }
  node->idx = idx[n-1];

  cilk_for (int i = 0; i < n-1; i++){
    distance[i]=distanceSer((X+i*d),node->vp,d);
  }

  md = quickSelect(distance, n - 1, (int)((n - 2) / 2));
  node->md = md;

  outerNum = (int)((n - 1) / 2);
  innerNum = n - 1 - outerNum;

  if (innerNum != 0) {
    Xinner = (double * ) malloc(innerNum * d * sizeof(double));
    innerIdx = (int *) malloc(innerNum * sizeof(int));
  }
  if (outerNum != 0) {
    Xouter = (double * ) malloc(outerNum * d * sizeof(double));
    outerIdx = (int *) malloc(outerNum * sizeof(int));
  }


  for (int i = 0; i < n - 1; i++) {
    if (distance[i] <= md) {
      for (int j = 0; j < d; j++) {
        *(Xinner + inCounter * d + j) = * (X + i * d + j);
      }
      innerIdx[inCounter]=idx[i];
      inCounter++;
    } else {
      for (int j = 0; j < d; j++) {
        *(Xouter + outCounter * d + j) = * (X + i * d + j);
      }
      outerIdx[outCounter]=idx[i];
      outCounter++;
    }
  }

    node->inner = cilk_spawn subTreeRec(Xinner, innerIdx, innerNum, d);
    node->outer = subTreeRec(Xouter, outerIdx, outerNum, d);

    free(distance);
    free(X);
    free(idx);
  cilk_sync;

  return node;
}


vptree * buildvp(double * X, int n, int d) {

  int * idx = (int *) malloc(n * sizeof(int));
   for(int i=0; i<n; i++){
     idx[i] = i;
   }
   return subTreeRec(X, idx, n, d);
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
