#ifndef VECTOR_UTIL_H
#define VECTOR_UTIL_H

#include <math.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include "logfile.h"

using namespace std;

//FROM SEQUENCER CODE
std::vector<double>  linear_interp(const std::vector<double> &xi, const std::vector<double> &yi,const std::vector<double> &xf);
double linear_interp(const std::vector<double> &xi, const std::vector<double> &yi,double xf);
std::vector<double> gauss_grid(const std::vector<double> &grid, double A, double sigma, double x0);

//FROM GAMMA CODE
unsigned long int vector_util_count_true(const std::vector<bool> &a);
std::vector<unsigned long int> vector_util_find(const std::vector<bool>& a);
std::vector<bool> vector_util_not(const std::vector<bool> &a);

//TEMPLATES MERGED FROM BOTH

//return with index at maximum element value
template <typename T>
unsigned long int vector_util_imax(const std::vector<T>&  var)
{
  if (var.size()==0) {
    logfile_error("vector_util.h : vector_util_imax : vector size is zero\n"); 
    return 0;
  }
  T maxX = var.at(0);
  unsigned long int max_i=0;
  for (unsigned long int ll =0; ll<var.size(); ++ll) {
    if ( maxX < var.at(ll) ) {
      maxX = var.at(ll);
      max_i = ll;
    }
  }
  return max_i;
}

//return with index at minimum element value
template <typename T>
unsigned long int vector_util_imin(const std::vector<T>&  var)
{
  if (var.size()==0) {
    logfile_error("vector_util.h : vector_util_imin : vector size is zero\n"); 
    return 0;
  }
  T minX = var.at(0);
  unsigned long int min_i=0;
  for (unsigned long int ll =0; ll<var.size(); ++ll) {
    if ( minX > var.at(ll) ) {
      minX = var.at(ll);
      min_i = ll;
    }
  }
  return min_i;
}

//return with index at minimum element value that is not at index n
//Vector size should be greater than one to allow for one excluded value
template <typename T>
unsigned long int vector_util_imin_excl_n(const std::vector<T>&  var, unsigned long int n)
{
  if (var.size()==0) {
    logfile_error("vector_util.h : vector_util_imin_excl_n : vector size is zero\n"); 
    return 0;
  }
  if (var.size()==1) {
    logfile_error("vector_util.h : vector_util_imin_excl_n : vector size is one\n"); 
    return 0;
  }
  if (logfile_success() == false) {
    logfile_error("vector_util.h : vector_util_imin_excl_n : success variable is set to false\n"); 
    return 0;  
  }
  unsigned long int min_i=0; //vector_util_imax(var);
  if (n==0) min_i =1;
  T minX = var.at(min_i);
  for (unsigned long int ll =0; ll<var.size(); ++ll) {
    if ( minX > var.at(ll) && ll != n) {
      minX = var.at(ll);
      min_i = ll;
    }
  }
  if (min_i==n) {
    logfile_error("vector_util.h : imin_excl_n : could not exclude index n\n"); 
    return 0;
  }
  return min_i;
}

//return with index at element value that is closest to var
template <typename T>
unsigned long int find_closest(const std::vector<T>&  var, T spec)
{
  if (var.size()==0) {
    logfile_error("vector_util.h : find_closest : vector size is zero\n"); 
    return 0;
  }
  T diff = (var.at(0)-spec);
  if (diff<0) diff = -diff;
  unsigned long int min_i=0;
  for (unsigned long int ll =0; ll<var.size(); ++ll) {
    T tmp_diff = var.at(ll)-spec;
    if (tmp_diff<0) tmp_diff = - tmp_diff;
    if ( tmp_diff< diff) {
      diff = tmp_diff;
      min_i = ll;
    }
  }
  return min_i;
}

template <typename T>
std::vector<T> operator+(const std::vector<T>& a, const std::vector<T>& b)
{
    std::vector<T> result;
    if (a.size() != b.size()) {
      logfile_error("vector_util.h : operator+ : vector size mismatch %u %u\n", a.size(),b.size() ); 
      return result;
    }
    result.reserve(a.size());

    std::transform(a.begin(), a.end(), b.begin(), 
                   std::back_inserter(result), std::plus<T>());    
    return result;
}

template <typename T>
std::vector<T> operator-(const std::vector<T>& a, const std::vector<T>& b)
{
    std::vector<T> result;
    if (a.size() != b.size()) {
      logfile_error("vector_util.h : operator- : vector size mismatch %u %u\n", a.size(),b.size() ); 
      return result;
    }
    result.reserve(a.size());

    std::transform(a.begin(), a.end(), b.begin(), 
                   std::back_inserter(result), std::minus<T>());
    return result;
}

template <typename T>
std::vector<T> operator/(const std::vector<T>& a, const std::vector<T>& b)
{
    std::vector<T> result;
    if (a.size() != b.size()) {
      logfile_error("vector_util.h : operator/ : vector size mismatch %u %u\n", a.size(),b.size() ); 
      return result;
    }
    result.reserve(a.size());
    std::transform(a.begin(), a.end(), b.begin(), 
                   std::back_inserter(result), std::divides<T>());
    return result;
}

template <typename T>
std::vector<T> operator*(const std::vector<T>& a, const std::vector<T>& b)
{
    std::vector<T> result;
    if (a.size() != b.size()) {
      logfile_error("vector_util.h : operator* : vector size mismatch %u %u\n", a.size(),b.size() ); 
      return result;
    }
    result.reserve(a.size());
    std::transform(a.begin(), a.end(), b.begin(), 
                   std::back_inserter(result), std::multiplies<T>());
    return result;
}

template <typename T, typename T2>
std::vector<T> operator/(const std::vector<T>& a, T2 b)
{
    std::vector<T> result;
    result.reserve(a.size());

    for (unsigned long int ii=0; ii<a.size(); ++ii) 
      result.push_back(a[ii]/T(b));
    return result;
}

template <typename T, typename T2>
std::vector<T> operator*(const std::vector<T>& a, T2 b)
{
    std::vector<T> result;
    result.reserve(a.size());

    for (unsigned long int ii=0; ii<a.size(); ++ii) 
      result.push_back(a[ii]*T(b));
    return result;
}

template <typename T, typename T2>
std::vector<T> operator+(const std::vector<T>& a, T2 b)
{
    std::vector<T> result;
    result.reserve(a.size());

    for (unsigned long int ii=0; ii<a.size(); ++ii) 
      result.push_back(a[ii]+T(b));
    return result;
}

template <typename T, typename T2>
std::vector<T> operator-(const std::vector<T>& a, T2 b)
{
    std::vector<T> result;
    result.reserve(a.size());

    for (unsigned long int ii=0; ii<a.size(); ++ii) 
      result.push_back(a[ii]-T(b));
    return result;
}

template <typename T, typename T2>
std::vector<T> operator+(T2 b, const std::vector<T>& a)
{
    std::vector<T> result;
    result.reserve(a.size());

    for (unsigned long int ii=0; ii<a.size(); ++ii) 
      result.push_back(T(b)+a[ii]);
    return result;
}

template <typename T, typename T2>
std::vector<T> operator-(T2 b, const std::vector<T>& a)
{
    std::vector<T> result;
    result.reserve(a.size());

    for (unsigned long int ii=0; ii<a.size(); ++ii) 
      result.push_back(T(b)-a[ii]);
    return result;
}

template <typename T, typename T2>
std::vector<T> operator*(T2 a, const std::vector<T>& b)
{
    return b*a;
}


template <typename T>
std::vector<T> vector_util_avg(const std::vector<T>& a, const std::vector<T>& b)
{
  std::vector<T> result = (a+b)/2.0;
  return result;
}

template <typename T>
std::vector<T> vector_util_abs(const std::vector<T>& a)
{
    std::vector<T> result;
    result.reserve(a.size());

    for (unsigned long int ii=0; ii<a.size(); ++ii)
      result.push_back( fabs(a[ii]) );
    return result;
}


//TEMPLATES FROM GAMMA ONLY

template <typename T>
unsigned long int numel(const std::vector<T>& a)
{
  return a.size();
}

template <typename T>
void vector_util_print(const std::vector<T>& a)
{  
  for (unsigned long int ii=0; ii<a.size(); ++ii) {
    logfile_cout(a[ii]);
    if (ii<a.size()-1)
      logfile_cout(',');
  }
  logfile_cout('\n');

}


template <typename T>
std::vector<T> vector_util_get(const std::vector<T>& a, const std::vector<bool>& b)
{
  std::vector<T> result;
  if (a.size() != b.size()) {
    logfile_error("vector_util.h : vector_util_get : vector size mismatch %u %u\n", a.size(),b.size() ); 
    return result; 
  }
  for (unsigned long int ii=0; ii<a.size(); ++ii) {
    if (b[ii]) result.push_back(a[ii]);
  }
  return result;
}

template <typename T>
T vector_util_get(const std::vector<T> &x, const unsigned long int N) { 
  if (N >= x.size() || N<0) {   
    logfile_error("vector_util.h : vector_util_get : index exceeds vector dimension %u %u\n", x.size(),N ); 
    return 0;
  }
  return x.at(N);
}

template <typename T>
std::vector<T> vector_util_get(const std::vector<T>& a, const std::vector<unsigned long int>& b)
{
  std::vector<T> result;
  result.reserve(b.size());
  for (unsigned long int ii=0; ii<b.size(); ++ii) {
    if (b[ii] >= a.size() || b[ii]<0) {   
      logfile_error("vector_util.h : vector_util_get : index exceeds vector dimension %u %u\n", a.size(),b[ii] ); 
      result.clear();
      return result;
    }
    result.push_back(a.at(b[ii]));
  }
  return result;
}


//These two functions are incompatible.  Why?  they're not used so just comment out for now
/* template <typename T, typename T2> */
/* void vector_util_set(std::vector<T>& a, const unsigned long int b, T2 val) */
/* { */
/*     if (b >= a.size() || b<0) {    */
/*       logfile_error("vector_util.h : vector_util_set : index exceeds vector dimension %u %u\n", a.size(),b );  */
/*       return ; */
/*     } */
/*     a.at(b) = (T) val; */
/* } */
/* template <typename T, typename T2> */
/* void vector_util_set(std::vector<T>& a, const std::vector<unsigned long int>& b, T2 val) */
/* { */
/*   for (unsigned long int ii=0; ii<b.size(); ++ii) { */
/*     if (b[ii] >= a.size() || b[ii]<0) {    */
/*       logfile_error("vector_util.h : vector_util_set : index exceeds vector dimension %u %u\n", a.size(),b[ii] );  */
/*       return ; */
/*     } */
/*     a.at(b[ii]) = (T) val; */
/*   } */
/* } */

template <typename T, typename T2>
void vector_util_set(std::vector<T>& a, const std::vector<bool>& b, T2 val)
{
  if (a.size() != b.size()) {
    logfile_error("vector_util.h : vector_util_set : vector size mismatch %u %u\n", a.size(),b.size() ); 
    return;
  }
  for (unsigned long int ii=0; ii<b.size(); ++ii) {
    if (b[ii]) a[ii] = (T) val;
  }
}

template <typename T, typename T2>
void vector_util_set(std::vector<T>& a, const std::vector<bool>& b, const std::vector<T2>& val)
{
  if (a.size() != b.size() ) {
    logfile_error("vector_util.h : vector_util_set : vector size mismatch %u %u \n", a.size(),b.size() ); 
    return;
  }
  if ( vector_util_count_true(b)  != val.size()) {
    logfile_error("vector_util.h : vector_util_set : vector size mismatch %u %u \n", vector_util_count_true(b),val.size() ); 
    return;
  }
  unsigned long int ctr = 0;
  for (unsigned long int ii=0; ii<b.size(); ++ii) {
    if (b[ii]) {
      a[ii] = (T) val.at(ctr);
      ctr +=1;
    }
  }
}

template <typename T, typename T2>
void vector_util_set(std::vector<T>& a, T2 val)
{
  for (unsigned long int ii=0; ii<a.size(); ++ii)
    a[ii] = (T) val;
}


template <typename T, typename T2>
void vector_util_set(std::vector<T> &x, const unsigned long int N, T2 val) { 
  if (N>= x.size() || N<0) {
    logfile_error("vector_util.h : vector_util_set : index exceeds vector dimension %u %u\n", x.size(),N ); 
    return;
  }
  x.at(N) = (T2) val;
}


template <typename T>
std::vector<bool> operator<(const std::vector<T>& a, const std::vector<T>& b)
{
    std::vector<bool> result;
    if (a.size() != b.size()) {
      logfile_error("vector_util.h : operator< : vector size mismatch %u %u\n", a.size(),b.size() ); 
      return result;
    }
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]<b[ii]);
    return result;
}

template <typename T, typename T2>
std::vector<bool> operator<(const std::vector<T>& a, T2 b)
{
    std::vector<bool> result;
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]<T(b));
    return result;
}

template <typename T, typename T2>
std::vector<bool> operator<(T2 a, const std::vector<T>& b)
{
    std::vector<bool> result;
    result.reserve(b.size());
    for (unsigned long int ii=0; ii<b.size(); ++ii) result.push_back(T(a)<b[ii]);
    return result;
}

template <typename T>
std::vector<bool> operator<=(const std::vector<T>& a, const std::vector<T>& b)
{
    std::vector<bool> result;
    if (a.size() != b.size()) {
      logfile_error("vector_util.h : operator<= : vector size mismatch %u %u\n", a.size(),b.size() ); 
      return result;
    }
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]<=b[ii]);
    return result;
}

template <typename T, typename T2>
std::vector<bool> operator<=(const std::vector<T>& a, T2 b)
{
    std::vector<bool> result;
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]<=T(b));
    return result;
}

template <typename T, typename T2>
std::vector<bool> operator<=(T2 a, const std::vector<T>& b)
{
    std::vector<bool> result;
    result.reserve(b.size());
    for (unsigned long int ii=0; ii<b.size(); ++ii) result.push_back(T(a)<=b[ii]);
    return result;
}


template <typename T>
std::vector<bool> operator>(const std::vector<T>& a, const std::vector<T>& b)
{
    std::vector<bool> result;
    if (a.size() != b.size()) {
      logfile_error("vector_util.h : operator> : vector size mismatch %u %u\n", a.size(),b.size() ); 
      return result;
    }
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]>b[ii]);
    return result;
}

template <typename T, typename T2>
std::vector<bool> operator>(const std::vector<T>& a, T2 b)
{
    std::vector<bool> result;
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]>T(b));
    return result;
}

template <typename T, typename T2>
std::vector<bool> operator>(T2 a, const std::vector<T>& b)
{
    std::vector<bool> result;
    result.reserve(b.size());
    for (unsigned long int ii=0; ii<b.size(); ++ii) result.push_back(T(a)>b[ii]);
    return result;
}

template <typename T>
std::vector<bool> operator>=(const std::vector<T>& a, const std::vector<T>& b)
{
    std::vector<bool> result;
    if (a.size() != b.size()) {
      logfile_error("vector_util.h : operator>= : vector size mismatch %u %u\n", a.size(),b.size() ); 
      return result;
    }
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]>=b[ii]);
    return result;
}

template <typename T, typename T2>
std::vector<bool> operator>=(const std::vector<T>& a, T2 b)
{
    std::vector<bool> result;
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]>=T(b));
    return result;
}

template <typename T, typename T2>
std::vector<bool> operator>=(T2 a, const std::vector<T>& b)
{
    std::vector<bool> result;
    result.reserve(b.size());
    for (unsigned long int ii=0; ii<b.size(); ++ii) result.push_back(T(a)>=b[ii]);
    return result;
}

template <typename T>
std::vector<bool> operator==(const std::vector<T>& a, const std::vector<T>& b)
{
    std::vector<bool> result;
    if (a.size() != b.size()) {
      logfile_error("vector_util.h : operator== : vector size mismatch %u %u\n", a.size(),b.size() ); 
      return result;
    }
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]==b[ii]);
    return result;
}

template <typename T, typename T2>
std::vector<bool> operator==(const std::vector<T>& a, T2 b)
{
    std::vector<bool> result;
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]==T(b));
    return result;
}

template <typename T, typename T2>
std::vector<bool> operator==(T2 a, const std::vector<T>& b)
{
    std::vector<bool> result;
    result.reserve(b.size());
    for (unsigned long int ii=0; ii<b.size(); ++ii) result.push_back(T(a)==b[ii]);
    return result;
}

template <typename T>
std::vector<bool> operator!=(const std::vector<T>& a, const std::vector<T>& b)
{
    std::vector<bool> result;
    if (a.size() != b.size()) {
      logfile_error("vector_util.h : operator!= : vector size mismatch %u %u\n", a.size(),b.size() ); 
      return result;
    }
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]!=b[ii]);
    return result;
}

template <typename T, typename T2>
std::vector<bool> operator!=(const std::vector<T>& a, T2 b)
{
    std::vector<bool> result;
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]!=T(b));
    return result;
}

template <typename T, typename T2>
std::vector<bool> operator!=(T2 a, const std::vector<T>& b)
{
    std::vector<bool> result;
    result.reserve(b.size());
    for (unsigned long int ii=0; ii<b.size(); ++ii) result.push_back(T(a)!=b[ii]);
    return result;
}

template <typename T>
std::vector<bool> operator&&(const std::vector<T>& a, const std::vector<T>& b)
{
    std::vector<bool> result;
    if (a.size() != b.size()) {
      logfile_error("vector_util.h : operator&& : vector size mismatch %u %u\n", a.size(),b.size() ); 
      return result;
    }
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]&&b[ii]);
    return result;
}

template <typename T>
std::vector<bool> operator&&(const std::vector<T>& a, bool b)
{
    std::vector<bool> result;
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]&&b);
    return result;
}

template <typename T>
std::vector<bool> operator&&(bool a, const std::vector<T>& b)
{
    std::vector<bool> result;
    result.reserve(b.size());
    for (unsigned long int ii=0; ii<b.size(); ++ii) result.push_back(a&&b[ii]);
    return result;
}

template <typename T>
std::vector<bool> operator||(const std::vector<T>& a, const std::vector<T>& b)
{
    std::vector<bool> result;
    if (a.size() != b.size()) {
      logfile_error("vector_util.h : operator|| : vector size mismatch %u %u\n", a.size(),b.size() ); 
      return result;
    }
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]||b[ii]);
    return result;
}

template <typename T>
std::vector<bool> operator||(const std::vector<T>& a, bool b)
{
    std::vector<bool> result;
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(a[ii]||b);
    return result;
}

template <typename T>
std::vector<bool> operator||(bool a, const std::vector<bool>& b)
{
    std::vector<bool> result;
    result.reserve(b.size());
    for (unsigned long int ii=0; ii<b.size(); ++ii) result.push_back(a||b[ii]);
    return result;
}






template <typename T>
T vector_util_sum(const std::vector<T>& var)
{

  T sum = 0;
  for (unsigned long int ll =0; ll<var.size(); ++ll) 
    sum += var[ll];
  return sum;
}

template <typename T>
T vector_util_min(const std::vector<T>& var)
{
  T minX = 0;
  if (var.size()>0) minX = var[0];
  else  logfile_error("vector_util.h : vector_util_min : vector size is zero\n"); 
  for (unsigned long int ll =0; ll<var.size(); ++ll) { 
    if ( minX > var[ll])   minX = var[ll];
  }
  return minX;
}

template <typename T>
T vector_util_max(const std::vector<T>& var)
{
  T maxX = 0;
  if (var.size()>0) maxX = var[0];
  else  logfile_error("vector_util.h : vector_util_max : vector size is zero\n"); 
  for (unsigned long int ll =0; ll<var.size(); ++ll) { 
    if ( maxX < var[ll])   maxX = var[ll];
  }
  return maxX;
}

template <typename T, typename T2>
std::vector<T> vector_util_min(const std::vector<T>& var, T2 b)
{
  std::vector<T> res = var;
  T minX = (T) b;
  for (unsigned long int ll =0; ll<var.size(); ++ll) { 
    if ( minX < var[ll])   res[ll] = minX;
  }
  return res;
}

template <typename T, typename T2>
std::vector<T> vector_util_max(const std::vector<T>& var, T2 b)
{
  std::vector<T> res = var;
  T maxX = (T) b;
  for (unsigned long int ll =0; ll<var.size(); ++ll) { 
    if ( maxX > var[ll])   res[ll]=maxX;
  }
  return res;
}

template <typename T, typename T2>
std::vector<T> vector_util_min( T2 b, const std::vector<T>& var)
{
  std::vector<T> res = var;
  T minX = (T) b;
  for (unsigned long int ll =0; ll<var.size(); ++ll) { 
    if ( minX < var[ll])   res[ll] = minX;
  }
  return res;
}

template <typename T, typename T2>
std::vector<T> vector_util_max(T2 b, const std::vector<T>& var)
{
  std::vector<T> res = var;
  T maxX = (T) b;
  for (unsigned long int ll =0; ll<var.size(); ++ll) { 
    if ( maxX > var[ll])   res[ll]=maxX;
  }
  return res;
}



template <typename T>
T vector_util_std(const std::vector<T>& var){
  if (var.size()==0) { 
    logfile_error("vector_util.h : vector_util_std : vector size is zero\n"); 
    return 0;
  }
  T NN=0.0; T stdev = 0.0; T stdev2= 0.0;
  for (unsigned long int ll =0; ll<var.size(); ++ll) { 
    stdev+= var[ll];
    stdev2+=var[ll]*var[ll];
    NN+=1.0;
  }
  stdev=stdev*stdev/NN;
  stdev=sqrt(fabs(stdev2-stdev)/(NN-1));
  return stdev;
}

template <typename T>
T vector_util_avg(const std::vector<T>& var){
  if (var.size()==0) { 
    logfile_error("vector_util.h : vector_util_avg : vector size is zero\n"); 
    return 0;
  }
  return (vector_util_sum(var)/ (T) var.size() );
}


template <typename T>
std::vector<T> vector_util_sqrt(const std::vector<T> &a) {
  std::vector<T> result=a;
  for (unsigned long int ii=0; ii<a.size(); ++ii) result[ii]=sqrt(a[ii]);
  return result;
}

template <typename T>
std::vector<T> vector_util_square(const std::vector<T> &a) {
  std::vector<T> result=a;
  for (unsigned long int ii=0; ii<a.size(); ++ii) result[ii]=a[ii]*a[ii];
  return result;
}

//find the slope of an x-y scatter plot assuming constant error bars
template <typename T>
T vector_util_slope(const std::vector<T>& x, const std::vector<T>& y) {
  if (x.size()==0) { 
    logfile_error("vector_util.h : vector_util_slope : vector size is zero\n"); 
    return 0;
  }
  if (x.size() != y.size()) {
    logfile_error("vector_util.h : vector_util_slope : vector size mismatch %u %u\n", x.size(), y.size()); 
    return 0;
  }
  if (x.size()==1) return 0; //fitting a line to a point
  T s1 = 0;  T s2 = 0;  T s3 = 0;  T s4 = 0;
  for (unsigned long int ll=0; ll<x.size(); ++ll) {
    s1+=x[ll]*y[ll];
    s2+=pow(x[ll],2);
    s3+= x[ll];
    s4+= y[ll];
  }
  return (s1-s3*s4/T(x.size()))/(s2-s3*s3/T(x.size()));
}


//find the slope of an x-y scatter plot constrained to pass through (0,0) and assuming constant error bars
template <typename T>
T vector_util_slope_no_offset(const std::vector<T>& x, const std::vector<T>& y) {
  if (x.size()==0) { 
    logfile_error("vector_util.h : vector_util_slope_no_offset : vector size is zero\n"); 
    return 0;
  }
  if (x.size() != y.size()) {
    logfile_error("vector_util.h : vector_util_slope_no_offset : vector size mismatch %u %u\n", x.size(), y.size()); 
    return 0;
  }
  if (x.size()==1) return 0; //fitting a line to a point
  T s1 = 0;
  T s2 = 0;
  for (unsigned long int ll=0; ll<x.size(); ++ll) {
    s1+=x[ll]*y[ll];
    s2+=pow(x[ll],2);
  }
  return (s1/s2);
}

//find the offset of an x-y scatter plot assuming constant error bars
template <typename T>
T vector_util_offset(const std::vector<T>& x, const std::vector<T>& y) {
  if (x.size()==0) { 
    logfile_error("vector_util.h : vector_util_offset : vector size is zero\n"); 
    return 0;
  }
  if (x.size() != y.size()) {
    logfile_error("vector_util.h : vector_util_soffset : vector size mismatch %u %u\n", x.size(), y.size()); 
    return 0;
  }
  T slope = vector_util_slope(x,y);
  T s3 = 0;  T s4 = 0;
  for (unsigned long int ll=0; ll<x.size(); ++ll) {
    s3+= x[ll];
    s4+= y[ll];
  }
  return  s4/T(x.size())-slope*s3/T(x.size());
}


template <typename T>
std::vector<T> vector_util_floor(const std::vector<T> &f) {
  std::vector<T> res;
  for (unsigned long int ii=0; ii<f.size(); ++ii) 
    res.push_back( (T) floor(f[ii]));
  return res;
}

template <typename T>
std::vector<T> vector_util_ceil(const std::vector<T> &f) {
  std::vector<T> res;
  for (unsigned long int ii=0; ii<f.size(); ++ii) 
    res.push_back( (T) ceil(f[ii]));
  return res;
}

template <typename T>
std::vector<float> vector_util_float(const std::vector<T> &f) {
  std::vector<float> res;
  res.reserve(f.size());
  for (unsigned long int ii=0; ii<f.size(); ++ii) 
    res.push_back( (float) f[ii]);
  return res;
}

template <typename T>
std::vector<double> vector_util_double(const std::vector<T> &f) {
  std::vector<double> res;
  res.reserve(f.size());
  for (unsigned long int ii=0; ii<f.size(); ++ii) 
    res.push_back( (double) f[ii]);
  return res;
}

template <typename T>
std::vector<bool> vector_util_isnan(const std::vector<T> &a)
{
    std::vector<bool> result;
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(isnan(a[ii]));
    return result;
}

template <typename T>
std::vector<bool> vector_util_not_isnan(const std::vector<T> &a)
{
    std::vector<bool> result;
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(!(isnan(a[ii])));
    return result;
}


template <typename T, typename T2>
void vector_util_create(std::vector<T> &x, const unsigned long int N, T2 val) { //creates matrix and sets all values to val
  x.clear();
  x.resize(N, (T) val);
}

template <typename T>
void vector_util_ones(std::vector<T> &x, const unsigned long int N) { //creates matrix and sets all values to one
  x.clear();
  x.resize(N, (T) 1);
}

template <typename T>
void vector_util_ones(std::vector<T> &x) { //sets all values to one
  for (unsigned long int ii=0;  ii<x.size(); ++ii)
    x[ii]=1;
}

template <typename T>
void vector_util_zeros(std::vector<T> &x, const unsigned long int N) { //creates matrix and sets all values to zeros
  x.clear();
  x.resize(N, (T) 0);
}

template <typename T>
void vector_util_zeros(std::vector<T> &x) { //sets all values to zeros
  for (unsigned long int ii=0;  ii<x.size(); ++ii)
    x[ii]=0;
}

template <typename T>
void vector_util_ctr(std::vector<T> &x, const unsigned long int N) { //creates matrix and sets all values to 1,2,3...
  x.clear();
  x.resize(N, (T) 0);
  for (unsigned long int ii=0;  ii<N; ++ii)
    x[ii]=T(ii+1);
}

template <typename T>
void vector_util_ctr(std::vector<T> &x) { //creates matrix and sets all values to 1,2,3...
  for (unsigned long int ii=0;  ii<x.size(); ++ii)
    x[ii]=T(ii+1);
}

template <typename T>
T vector_util_end(const std::vector<T> &x) { 
  if (x.size()==0) {
    logfile_error("vector_util.h : vector_util_end : vector size is zero\n"); 
    return 0;
  }
  return *(x.end()-1);
}

template <typename T>
T vector_util_begin(const std::vector<T> &x) { 
  if (x.size()==0) {
    logfile_error("vector_util.h : vector_util_begin : vector size is zero\n"); 
    return 0;
  }
  return x.at(0);
}

template <typename T>
void vector_util_cat(std::vector<T> &x, const std::vector<T> &y) { 
  x.insert(x.end(),y.begin(),y.end() );
}

//returns with the step size between elements assuming constant spacing between elements
template <typename T>
T vector_util_step_size(const std::vector<T>& a)
{
  if (a.size()==0) {
    logfile_error("vector_util.h : vector_util_step_size : vector size is zero\n"); 
    return 0;
  }
  if (a.size()==1) return 1;
  T step_size = (vector_util_end(a)-vector_util_begin(a))/(float(numel(a))-1.0);
  if (step_size == 0) return 0;
  for (unsigned long int ii=0; ii<a.size()-1; ++ii) {
    if (  (a.at(ii+1)-a.at(ii))/step_size < 0.9 || (a.at(ii+1)-a.at(ii))/step_size > 1.1) {
      logfile_error("vector_util.h : vector_util_step_size : step size is not uniform\n"); 
      return 0;
    }
  }
  return (vector_util_end(a)-vector_util_begin(a))/(float(numel(a))-1);
}

template <typename T>
unsigned long int size(const std::vector<T> f) {
  logfile_printf("numel(f) = %u\n", f.size());
  return f.size();
}



#endif 

