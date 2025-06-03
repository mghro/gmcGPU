#include "LM_cubicspline.h"

Search_Matrices<float> LM_cubicspline(const std::vector<float> &xi, const std::vector<float> &yi, 
				       const std::vector<float> &zi, const Mesh_Grid<float> &mesh0, 
				       const float *alpha, unsigned long int size_alpha, bool flag_1st_deriv, double t_start) {
#ifdef TIME_COUT
  logfile_printf(" time LM_cubicspline 1 =  %f\n", double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif
  Search_Matrices<float> sm;
  if (size_alpha != xi.size()*yi.size()*zi.size()*64
      || mesh0.y.M.size() != mesh0.x.M.size() ||mesh0.z.M.size() != mesh0.x.M.size() 
      || xi.size()==0 || yi.size()==0 || zi.size() == 0 || mesh0.x.M.size() == 0) {
    logfile_error("LM_cubicspline.cpp : LM_cubicspline : matrix improper format\n");
    size(xi);
    size(yi);
    size(zi);
    size(mesh0.x);
    size(mesh0.y);
    size(mesh0.z);
    logfile_printf("%i\n", size_alpha);
    return sm;
  }

  unsigned long int out_Lx = mesh0.x.nx;
  unsigned long int out_Ly = mesh0.x.ny;
  unsigned long int out_Lz = mesh0.x.nz;
#ifdef TIME_COUT
  logfile_printf(" time LM_cubicspline 2 =  %f\n", double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif
  sm = LM_cubicspline(xi,yi,zi,mesh0.x.M,mesh0.y.M,mesh0.z.M,alpha,size_alpha,flag_1st_deriv, t_start);
#ifdef TIME_COUT
  logfile_printf(" time LM_cubicspline 8 =  %f\n", double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif
  sm.D1_interp.nx=out_Lx;
  sm.D1_interp.ny=out_Ly;
  sm.D1_interp.nz=out_Lz;
  if (sm.D1_interp.M.size() != sm.D1_interp.nx*sm.D1_interp.ny*sm.D1_interp.nz) {
    logfile_error("LM_cubicspline.cpp : LM_cubicspline : matrix resulting from spline interpolation has wrong number of elements\n");
    size(sm.D1_interp);
    Search_Matrices<float> tmp;
    return tmp;
  }
  if (flag_1st_deriv) {
    sm.fx.nx=out_Lx;
    sm.fx.ny=out_Ly;
    sm.fx.nz=out_Lz;
    sm.fy.nx=out_Lx;
    sm.fy.ny=out_Ly;
    sm.fy.nz=out_Lz;
    sm.fz.nx=out_Lx;
    sm.fz.ny=out_Ly;
    sm.fz.nz=out_Lz;
    if (sm.fx.M.size() != sm.fx.nx*sm.fx.ny*sm.fx.nz ||
	sm.fy.M.size() != sm.fy.nx*sm.fy.ny*sm.fy.nz ||
	sm.fz.M.size() != sm.fz.nx*sm.fz.ny*sm.fz.nz ) {
      logfile_error("LM_cubicspline.cpp : LM_cubicspline : matrix of derivatives resulting from spline interpolation has wrong number of elements\n");
      size(sm.fx);
      size(sm.fy);
      size(sm.fz);
      Search_Matrices<float> tmp;
      return tmp;
    } 
  }
  return sm;
}

Search_Matrices<float> LM_cubicspline(const std::vector<float> &xi, const std::vector<float> &yi, const std::vector<float> &zi, 
				       const std::vector<float> &xf, const std::vector<float> &yf, const std::vector<float> &zf, 
				       const float *alpha, unsigned long int size_alpha, bool flag_1st_deriv, double t_start)
{
#ifdef TIME_COUT
  logfile_printf(" time LM_cubicspline 4 =  %f\n", double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif

  Search_Matrices<float> sm;
  if (size_alpha != xi.size()*yi.size()*zi.size()*64
      ||yf.size() != xf.size() ||zf.size() != xf.size() 
      || xi.size()==0 || yi.size()==0 || zi.size() == 0 || xf.size() == 0) {
    logfile_error("LM_cubicspline.cpp : LM_cubicspline : matrix improper format\n");
    size(xi);
    size(yi);
    size(zi);
    size(xf);
    size(yf);
    size(zf);
    logfile_printf("%i\n", size_alpha);
    return sm;
  }
  if (logfile_success() == false) {
    logfile_error("LM_cubicspline.cpp : LM_cubicspline : exiting due to previous errors\n");
    return sm;
  }
  unsigned long int tig = xi.size()*yi.size()*zi.size();

  unsigned long int out_Lx = numel(xf);
  unsigned long int out_Ly = 1;
  unsigned long int out_Lz = 1;
 
  sm.D1_interp.M.reserve(out_Lx);
  sm.fx.M.reserve(out_Lx);
  sm.fy.M.reserve(out_Lx);
  sm.fz.M.reserve(out_Lx);

  unsigned long int  Lx = numel(xi);
  unsigned long int  Ly = numel(yi);
  unsigned long int  Lz = numel(zi);

  unsigned long int n1x=0;
  unsigned long int n1y=0;
  unsigned long int n1z=0;
  long int tmp;
  float dx,dy,dz;
  float nan = FP_NAN;
  
  if (Lx>1) 
    dx = vector_util_step_size(xi); 
  else dx =1;
  if (Ly>1) 
    dy = vector_util_step_size(yi); 
  else dy =1;
  if (Lz>1)
    dz = vector_util_step_size(zi); 
  else dz =1;

  if (logfile_success() == false) {
    logfile_error("LM_cubicspline.cpp : LM_cubicspline : exiting due to previous errors\n");
    return sm;
  }

#ifdef TIME_COUT
  logfile_printf(" time LM_cubicspline 5 =  %f\n", double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif
  for (unsigned long int ii=0; ii<out_Lx; ++ii) {
    float x=xf[ii];
    float y=yf[ii];
    float z=zf[ii];

    if (Lx>1) {
      //Determine bounding box (in units of grid spacings) surrouding (x,y,z)
      x=(x-xi[0])/dx;  //units of x,y,z are now [grid spacings] and begin at (0,0,0) and end at (Lx-1,Ly-1,Lz-1)
      tmp=(long int) floor(x);  
      tmp=std::max(tmp, (long int) 0 );
      tmp=std::min(tmp,(long int) (Lx-2));
      x=x-float(tmp);
      n1x = (unsigned long int) tmp;
    }
    else x=0;
    if (Ly>1) {
      y=(y-yi[0])/dy; 
      tmp=(long int) floor(y);  
      tmp=std::max(tmp,(long int) 0);
      tmp=std::min(tmp,(long int) (Ly-2));
      y=y-float(tmp);
      n1y = (unsigned long int) tmp;
    }
    else y=0;
    if (Lz>1) {
      z=(z-zi[0])/dz; 
      tmp=(long int) floor(z);  
      tmp=std::max(tmp,(long int) 0);
      tmp=std::min(tmp, (long int) (Lz-2));
      z=z-float(tmp);
      n1z = (unsigned long int) tmp;
    }
    else z=0;
    if (  x>-0.5 && x<1.5 && 
	  y>-0.5 && y<1.5 && 
	  z>-0.5 && z<1.5   ) {  //reasonable numbers, keep on going
      float x2 = x*x;
      float x3 = x*x2;
      float y2 = y*y;
      float y3 = y*y2;
      float z2 = z*z;
      float z3 = z*z2;
      //voxel number of the bounding box for each interpolated point
      unsigned long int id000= n1z*Ly*Lx+n1y*Lx+n1x; 
      if ( !flag_1st_deriv) {

	sm.D1_interp.M.push_back  // Spline interpolation
	  (
	   alpha[id000]                   + alpha[id000+tig]         *z + alpha[id000+2 *tig]      *z2 + alpha[id000+3 *tig]      *z3+
	   alpha[id000+4 *tig]   *y  + alpha[id000+5 *tig]   *y *z + alpha[id000+6 *tig]   *y *z2 + alpha[id000+7 *tig]   *y *z3+
	   alpha[id000+8 *tig]   *y2 + alpha[id000+9 *tig]   *y2*z + alpha[id000+10*tig]   *y2*z2 + alpha[id000+11*tig]   *y2*z3+
	   alpha[id000+12*tig]   *y3 + alpha[id000+13*tig]   *y3*z + alpha[id000+14*tig]   *y3*z2 + alpha[id000+15*tig]   *y3*z3+
	   alpha[id000+16*tig]*x     + alpha[id000+17*tig]*x    *z + alpha[id000+18*tig]*x    *z2 + alpha[id000+19*tig]*x    *z3+
	   alpha[id000+20*tig]*x *y  + alpha[id000+21*tig]*x *y *z + alpha[id000+22*tig]*x *y *z2 + alpha[id000+23*tig]*x *y *z3+
	   alpha[id000+24*tig]*x *y2 + alpha[id000+25*tig]*x *y2*z + alpha[id000+26*tig]*x *y2*z2 + alpha[id000+27*tig]*x *y2*z3+
	   alpha[id000+28*tig]*x *y3 + alpha[id000+29*tig]*x *y3*z + alpha[id000+30*tig]*x *y3*z2 + alpha[id000+31*tig]*x *y3*z3+
	   alpha[id000+32*tig]*x2    + alpha[id000+33*tig]*x2   *z + alpha[id000+34*tig]*x2   *z2 + alpha[id000+35*tig]*x2   *z3+
	   alpha[id000+36*tig]*x2*y  + alpha[id000+37*tig]*x2*y *z + alpha[id000+38*tig]*x2*y *z2 + alpha[id000+39*tig]*x2*y *z3+
	   alpha[id000+40*tig]*x2*y2 + alpha[id000+41*tig]*x2*y2*z + alpha[id000+42*tig]*x2*y2*z2 + alpha[id000+43*tig]*x2*y2*z3+
	   alpha[id000+44*tig]*x2*y3 + alpha[id000+45*tig]*x2*y3*z + alpha[id000+46*tig]*x2*y3*z2 + alpha[id000+47*tig]*x2*y3*z3+
	   alpha[id000+48*tig]*x3    + alpha[id000+49*tig]*x3   *z + alpha[id000+50*tig]*x3   *z2 + alpha[id000+51*tig]*x3   *z3+
	   alpha[id000+52*tig]*x3*y  + alpha[id000+53*tig]*x3*y *z + alpha[id000+54*tig]*x3*y *z2 + alpha[id000+55*tig]*x3*y *z3+
	   alpha[id000+56*tig]*x3*y2 + alpha[id000+57*tig]*x3*y2*z + alpha[id000+58*tig]*x3*y2*z2 + alpha[id000+59*tig]*x3*y2*z3+
	   alpha[id000+60*tig]*x3*y3 + alpha[id000+61*tig]*x3*y3*z + alpha[id000+62*tig]*x3*y3*z2 + alpha[id000+63*tig]*x3*y3*z3
	   );
      } //end of !flag_1st_deriv
      else { //1st derivative calc (i.e. calculate 1st derivatives of the spline interpolation)
	unsigned long int ctr = 0;
        float  a00=alpha[id000]; ctr +=1;
        float  a01=alpha[id000+tig]; ctr +=1;
        float  a02=alpha[id000+ctr*tig]; ctr +=1;
        float  a03=alpha[id000+ctr*tig]; ctr +=1;
        float  a04=alpha[id000+ctr*tig]; ctr +=1;
        float  a05=alpha[id000+ctr*tig]; ctr +=1;
        float  a06=alpha[id000+ctr*tig]; ctr +=1;
        float  a07=alpha[id000+ctr*tig]; ctr +=1;
        float  a08=alpha[id000+ctr*tig]; ctr +=1;
        float  a09=alpha[id000+ctr*tig]; ctr +=1;
        float  a10=alpha[id000+ctr*tig]; ctr +=1;
        float  a11=alpha[id000+ctr*tig]; ctr +=1;
        float  a12=alpha[id000+ctr*tig]; ctr +=1;
        float  a13=alpha[id000+ctr*tig]; ctr +=1;
        float  a14=alpha[id000+ctr*tig]; ctr +=1;
        float  a15=alpha[id000+ctr*tig]; ctr +=1;
        float  a16=alpha[id000+ctr*tig]; ctr +=1;
        float  a17=alpha[id000+ctr*tig]; ctr +=1;
        float  a18=alpha[id000+ctr*tig]; ctr +=1;
        float  a19=alpha[id000+ctr*tig]; ctr +=1;
        float  a20=alpha[id000+ctr*tig]; ctr +=1;
        float  a21=alpha[id000+ctr*tig]; ctr +=1;
        float  a22=alpha[id000+ctr*tig]; ctr +=1;
        float  a23=alpha[id000+ctr*tig]; ctr +=1;
        float  a24=alpha[id000+ctr*tig]; ctr +=1;
        float  a25=alpha[id000+ctr*tig]; ctr +=1;
        float  a26=alpha[id000+ctr*tig]; ctr +=1;
        float  a27=alpha[id000+ctr*tig]; ctr +=1;
        float  a28=alpha[id000+ctr*tig]; ctr +=1;
        float  a29=alpha[id000+ctr*tig]; ctr +=1;
        float  a30=alpha[id000+ctr*tig]; ctr +=1;
        float  a31=alpha[id000+ctr*tig]; ctr +=1;
        float  a32=alpha[id000+ctr*tig]; ctr +=1;
        float  a33=alpha[id000+ctr*tig]; ctr +=1;
        float  a34=alpha[id000+ctr*tig]; ctr +=1;
        float  a35=alpha[id000+ctr*tig]; ctr +=1;
        float  a36=alpha[id000+ctr*tig]; ctr +=1;
        float  a37=alpha[id000+ctr*tig]; ctr +=1;
        float  a38=alpha[id000+ctr*tig]; ctr +=1;
        float  a39=alpha[id000+ctr*tig]; ctr +=1;
        float  a40=alpha[id000+ctr*tig]; ctr +=1;
        float  a41=alpha[id000+ctr*tig]; ctr +=1;
        float  a42=alpha[id000+ctr*tig]; ctr +=1;
        float  a43=alpha[id000+ctr*tig]; ctr +=1;
        float  a44=alpha[id000+ctr*tig]; ctr +=1;
        float  a45=alpha[id000+ctr*tig]; ctr +=1;
        float  a46=alpha[id000+ctr*tig]; ctr +=1;
        float  a47=alpha[id000+ctr*tig]; ctr +=1;
        float  a48=alpha[id000+ctr*tig]; ctr +=1;
        float  a49=alpha[id000+ctr*tig]; ctr +=1;
        float  a50=alpha[id000+ctr*tig]; ctr +=1;
        float  a51=alpha[id000+ctr*tig]; ctr +=1;
        float  a52=alpha[id000+ctr*tig]; ctr +=1;
        float  a53=alpha[id000+ctr*tig]; ctr +=1;
        float  a54=alpha[id000+ctr*tig]; ctr +=1;
        float  a55=alpha[id000+ctr*tig]; ctr +=1;
        float  a56=alpha[id000+ctr*tig]; ctr +=1;
        float  a57=alpha[id000+ctr*tig]; ctr +=1;
        float  a58=alpha[id000+ctr*tig]; ctr +=1;
        float  a59=alpha[id000+ctr*tig]; ctr +=1;
        float  a60=alpha[id000+ctr*tig]; ctr +=1;
        float  a61=alpha[id000+ctr*tig]; ctr +=1;
        float  a62=alpha[id000+ctr*tig]; ctr +=1;
        float  a63=alpha[id000+ctr*tig]; ctr +=1;
		
	float W00=1 ;
        float W01=             z ;
        float W02=             z2;
        float W03=             z3;
        float W04=       y       ;
        float W05=       y    *z ;
        float W06=       y    *z2;
        float W07=       y    *z3;
        float W08=       y2      ;
        float W09=       y2   *z ;
        float W10=       y2   *z2;
        float W11=       y2   *z3;
        float W12=       y3      ;
        float W13=       y3   *z ;
        float W14=       y3   *z2;
        float W15=       y3   *z3;
        float W16= x             ;
        float W17= x          *z ;
        float W18= x          *z2;
        float W19= x          *z3;
        float W20= x    *y       ;
        float W21= x    *y    *z ;
        float W22= x    *y    *z2;
        float W23= x    *y    *z3;
        float W24= x    *y2      ;
        float W25= x    *y2   *z ;
        float W26= x    *y2   *z2;
        float W27= x    *y2   *z3;
        float W28= x    *y3      ;
        float W29= x    *y3   *z ;
        float W30= x    *y3   *z2;
        float W31= x    *y3   *z3;
        float W32= x2            ;
        float W33= x2         *z ;
        float W34= x2         *z2;
        float W35= x2         *z3;
        float W36= x2   *y       ;
        float W37= x2   *y    *z ;
        float W38= x2   *y    *z2;
        float W39= x2   *y    *z3;
        float W40= x2   *y2      ;
        float W41= x2   *y2   *z ;
        float W42= x2   *y2   *z2;
        float W43= x2   *y2   *z3;
        float W44= x2   *y3      ;
        float W45= x2   *y3   *z ;
        float W46= x2   *y3   *z2;
        float W47= x2   *y3   *z3;
        float W48= x3            ;
        float W49= x3         *z ;
        float W50= x3         *z2;
        float W51= x3         *z3;
        float W52= x3   *y       ;
        float W53= x3   *y    *z ;
        float W54= x3   *y    *z2;
        float W55= x3   *y    *z3;
        float W56= x3   *y2      ;
        float W57= x3   *y2   *z ;
        float W58= x3   *y2   *z2;
        float W59= x3   *y2   *z3;
        float W60= x3   *y3      ;
        float W61= x3   *y3   *z ;
        float W62= x3   *y3   *z2;
        float W63= x3   *y3   *z3;	
	sm.D1_interp.M.push_back
	  (
	   a00*W00+ a01*W01+ a02*W02+ a03*W03+
	   a04*W04+ a05*W05+ a06*W06+ a07*W07+
	   a08*W08+ a09*W09+ a10*W10+ a11*W11+
	   a12*W12+ a13*W13+ a14*W14+ a15*W15+
	   a16*W16+ a17*W17+ a18*W18+ a19*W19+
	   a20*W20+ a21*W21+ a22*W22+ a23*W23+
	   a24*W24+ a25*W25+ a26*W26+ a27*W27+
	   a28*W28+ a29*W29+ a30*W30+ a31*W31+
	   a32*W32+ a33*W33+ a34*W34+ a35*W35+
	   a36*W36+ a37*W37+ a38*W38+ a39*W39+
	   a40*W40+ a41*W41+ a42*W42+ a43*W43+
	   a44*W44+ a45*W45+ a46*W46+ a47*W47+
	   a48*W48+ a49*W49+ a50*W50+ a51*W51+
	   a52*W52+ a53*W53+ a54*W54+ a55*W55+
	   a56*W56+ a57*W57+ a58*W58+ a59*W59+
	   a60*W60+ a61*W61+ a62*W62+ a63*W63
	   );
	sm.fx.M.push_back((
		   W00 *a16+   W01 *a17+   W02 *a18+   W03 *a19+
	  	   W04 *a20+   W05 *a21+   W06 *a22+   W07 *a23+
		   W08 *a24+   W09 *a25+   W10 *a26+   W11 *a27+
		   W12 *a28+   W13 *a29+   W14 *a30+   W15 *a31+
		 2*W16 *a32+ 2*W17 *a33+ 2*W18 *a34+ 2*W19 *a35+
		 2*W20 *a36+ 2*W21 *a37+ 2*W22 *a38+ 2*W23 *a39+
		 2*W24 *a40+ 2*W25 *a41+ 2*W26 *a42+ 2*W27 *a43+
		 2*W28 *a44+ 2*W29 *a45+ 2*W30 *a46+ 2*W31 *a47+
		 3*W32 *a48+ 3*W33 *a49+ 3*W34 *a50+ 3*W35 *a51+
		 3*W36 *a52+ 3*W37 *a53+ 3*W38 *a54+ 3*W39 *a55+
		 3*W40 *a56+ 3*W41 *a57+ 3*W42 *a58+ 3*W43 *a59+
		 3*W44 *a60+ 3*W45 *a61+ 3*W46 *a62+ 3*W47 *a63
		 )/dx);						
	sm.fy.M.push_back((						
	          a04*W00+   a05*W01+   a06*W02+   a07*W03+
	        2*a08*W04+ 2*a09*W05+ 2*a10*W06+ 2*a11*W07+
	        3*a12*W08+ 3*a13*W09+ 3*a14*W10+ 3*a15*W11+
	          a20*W16+   a21*W17+   a22*W18+   a23*W19+
	        2*a24*W20+ 2*a25*W21+ 2*a26*W22+ 2*a27*W23+
	        3*a28*W24+ 3*a29*W25+ 3*a30*W26+ 3*a31*W27+
	          a36*W32+   a37*W33+   a38*W34+   a39*W35+
	        2*a40*W36+ 2*a41*W37+ 2*a42*W38+ 2*a43*W39+
	        3*a44*W40+ 3*a45*W41+ 3*a46*W42+ 3*a47*W43+
	          a52*W48+   a53*W49+   a54*W50+   a55*W51+
	        2*a56*W52+ 2*a57*W53+ 2*a58*W54+ 2*a59*W55+
	        3*a60*W56+ 3*a61*W57+ 3*a62*W58+ 3*a63*W59
		 )/dy);
	sm.fz.M.push_back((
	        a01*W00+ 2*a02*W01+ 3*a03*W02+
	        a05*W04+ 2*a06*W05+ 3*a07*W06+
	        a09*W08+ 2*a10*W09+ 3*a11*W10+
	        a13*W12+ 2*a14*W13+ 3*a15*W14+
	        a17*W16+ 2*a18*W17+ 3*a19*W18+
	        a21*W20+ 2*a22*W21+ 3*a23*W22+
	        a25*W24+ 2*a26*W25+ 3*a27*W26+
	        a29*W28+ 2*a30*W29+ 3*a31*W30+
	        a33*W32+ 2*a34*W33+ 3*a35*W34+
	        a37*W36+ 2*a38*W37+ 3*a39*W38+
	        a41*W40+ 2*a42*W41+ 3*a43*W42+
	        a45*W44+ 2*a46*W45+ 3*a47*W46+
	        a49*W48+ 2*a50*W49+ 3*a51*W50+
	        a53*W52+ 2*a54*W53+ 3*a55*W54+
	        a57*W56+ 2*a58*W57+ 3*a59*W58+
	        a61*W60+ 2*a62*W61+ 3*a63*W62
		 )/dz); 
	
      }  //end of 1st deriv calc
    } //end of checking for reasonable x,y,z
    else {// unreasonable numbers, insert nan
      //cout<<ii<<' '<<x<<' '<<y<<' '<<z<<endl;;
      sm.D1_interp.M.push_back(nan);
      if ( flag_1st_deriv) {
	sm.fx.M.push_back(nan);	
	sm.fy.M.push_back(nan);	
	sm.fz.M.push_back(nan);	
      }
    }
  }  //end looping over ii (elements in xf)
#ifdef TIME_COUT
  logfile_printf(" time LM_cubicspline 6 =  %f\n", double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif

 //set matrix sizes and check for the correct number of elements
  sm.D1_interp.nx=out_Lx;
  sm.D1_interp.ny=out_Ly;
  sm.D1_interp.nz=out_Lz;
  if (sm.D1_interp.M.size() != sm.D1_interp.nx*sm.D1_interp.ny*sm.D1_interp.nz) {
    logfile_error("LM_cubicspline.cpp : LM_cubicspline : matrix resulting from spline interpolation has wrong number of elements\n");
    size(sm.D1_interp);
    Search_Matrices<float> tmp;
    return tmp;
  }
  

 if( flag_1st_deriv) {
   sm.fx.nx=out_Lx;
   sm.fx.ny=out_Ly;
   sm.fx.nz=out_Lz;
   sm.fy.nx=out_Lx;
   sm.fy.ny=out_Ly;
   sm.fy.nz=out_Lz;
   sm.fz.nx=out_Lx;
   sm.fz.ny=out_Ly;
   sm.fz.nz=out_Lz;
   if (sm.fx.M.size() != sm.fx.nx*sm.fx.ny*sm.fx.nz ||
       sm.fy.M.size() != sm.fy.nx*sm.fy.ny*sm.fy.nz ||
       sm.fz.M.size() != sm.fz.nx*sm.fz.ny*sm.fz.nz ) {
     logfile_error("LM_cubicspline.cpp : LM_cubicspline : matrix of derivatives resulting from spline interpolation has wrong number of elements\n");
     size(sm.fx);
     size(sm.fy);
     size(sm.fz);
     Search_Matrices<float> tmp;
     return tmp;
   }
 }
#ifdef TIME_COUT
  logfile_printf(" time LM_cubicspline 7 =  %f\n", double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif
 
 return sm;
}
