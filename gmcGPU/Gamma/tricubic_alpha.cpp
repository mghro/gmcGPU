#include "tricubic_alpha.h"
#include <vector>
#include <iostream>
#include <iterator>
void tricubic_alpha(float *alpha, unsigned long int size_alpha, const std::vector<float> &xi, const std::vector<float> &yi, 
			     const std::vector<float> &zi, const Matrix<float> &fmat, double t_start)
{
  if (logfile_success() == false) { 
    logfile_error("tricubic_alpha.cpp : tricubic_alpha : exiting due to previous errors\n");
    return;
  }


#ifdef TIME_COUT
  logfile_printf(" time tricubic_alpha 1 =  %f\n" << double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif
  if (fmat.nx*fmat.ny*fmat.nz != fmat.M.size()
      || size_alpha != fmat.M.size()*64) {
    logfile_error("tricubic_alpha.cpp : tricubic_alpha : matrix improper format\n");
    logfile_printf("Size of fmat = "); size(fmat);
    logfile_printf("nx,ny,nz = %i %i %i\n", fmat.nx, fmat.ny,fmat.nz);
    logfile_printf("Size of alpha = %i", size_alpha);
    return ;
  }

#ifdef TIME_COUT
  logfile_printf(" time tricubic_alpha 1a =  %f\n" << double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif
  Derivatives<float> deriv = derivatives(xi, yi, zi, fmat);
  if (deriv.fx.M.size() <1) { 
    logfile_error("tricubic_alpha.cpp : tricubic_alpha : matrices of derivatives have zero size\n");
    return;
  }
#ifdef TIME_COUT
  logfile_printf(" time tricubic_alpha 1b =  %f\n" << double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif
  float dx =vector_util_step_size(xi);
  float dy =vector_util_step_size(yi);
  float dz =vector_util_step_size(zi);
  if (logfile_success() == false) { 
    logfile_error("tricubic_alpha.cpp : tricubic_alpha : exiting due to previous errors\n");
    return;
  }
  
#ifdef TIME_COUT
  logfile_printf(" time tricubic_alpha 1c =  %f\n" << double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif
  std::vector<float> f =   fmat.M;

  std::vector<float> fx =   deriv.fx.M*dx;
  std::vector<float> fy =   deriv.fy.M*dy;
  std::vector<float> fz =   deriv.fz.M*dz;
  std::vector<float> fxy =  deriv.fxy.M*(dx*dy);
  std::vector<float> fxz =  deriv.fxz.M*(dx*dz);
  std::vector<float> fyz =  deriv.fyz.M*(dy*dz);
  std::vector<float> fxyz = deriv.fxyz.M*(dx*dy*dz);
#ifdef TIME_COUT
  logfile_printf(" time tricubic_alpha 1d =  %f\n" << double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif
  unsigned long int N = fmat.M.size();
#ifdef TIME_COUT
  logfile_printf(" time tricubic_alpha 1e =  %f\n" << double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif

#ifdef TIME_COUT
  logfile_printf(" time tricubic_alpha 2 =  %f\n" << double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif
  for (unsigned long int kk=0; kk<fmat.nz; ++kk) {
    unsigned long int ctrz0 =      kk*fmat.ny*fmat.nx;
    unsigned long int ctrz1 = (unsigned long int) ((kk+1)*fmat.ny*fmat.nx)%(fmat.nz*fmat.ny*fmat.nx);
    for (unsigned long int jj=0; jj<fmat.ny; ++jj) {
      unsigned long int ctry0 =    jj*fmat.nx;
      unsigned long int ctry1 = (unsigned long int) ((jj+1)*fmat.nx)%(fmat.ny*fmat.nx);
      for (unsigned long int ii=0; ii<fmat.nx; ++ii) {
	unsigned long int ctrx0 =  ii;
	unsigned long int ctrx1 = (unsigned long int) ((ii+1))%fmat.nx;
	unsigned long int ctr = ctrx0+ctry0+ctrz0;

	//function values and gradients at the corners of the bounding box
        //size(f000)=(numel(xi),numel(yi),numel(zi)
	//cout<<ctrx0<<' '<<ctry0<<' '<<ctrz1<<endl;
        float      f000=    f[ctr];
        float      f001=    f[ctrx0 +ctry0 +ctrz1];
        float      f010=    f[ctrx0 +ctry1 +ctrz0];
        float      f011=    f[ctrx0 +ctry1 +ctrz1];
        float      f100=    f[ctrx1 +ctry0 +ctrz0];
        float      f101=    f[ctrx1 +ctry0 +ctrz1];
        float      f110=    f[ctrx1 +ctry1 +ctrz0];
        float      f111=    f[ctrx1 +ctry1 +ctrz1];
        float     fx000=   fx[ctr];
        float     fx001=   fx[ctrx0 +ctry0 +ctrz1];
        float     fx010=   fx[ctrx0 +ctry1 +ctrz0];
        float     fx011=   fx[ctrx0 +ctry1 +ctrz1];
        float     fx100=   fx[ctrx1 +ctry0 +ctrz0];
        float     fx101=   fx[ctrx1 +ctry0 +ctrz1];
        float     fx110=   fx[ctrx1 +ctry1 +ctrz0];
        float     fx111=   fx[ctrx1 +ctry1 +ctrz1];
        float     fy000=   fy[ctr];
        float     fy001=   fy[ctrx0 +ctry0 +ctrz1];
        float     fy010=   fy[ctrx0 +ctry1 +ctrz0];
        float     fy011=   fy[ctrx0 +ctry1 +ctrz1];
        float     fy100=   fy[ctrx1 +ctry0 +ctrz0];
        float     fy101=   fy[ctrx1 +ctry0 +ctrz1];
        float     fy110=   fy[ctrx1 +ctry1 +ctrz0];
        float     fy111=   fy[ctrx1 +ctry1 +ctrz1];
        float     fz000=   fz[ctr];
        float     fz001=   fz[ctrx0 +ctry0 +ctrz1];
        float     fz010=   fz[ctrx0 +ctry1 +ctrz0];
        float     fz011=   fz[ctrx0 +ctry1 +ctrz1];
        float     fz100=   fz[ctrx1 +ctry0 +ctrz0];
        float     fz101=   fz[ctrx1 +ctry0 +ctrz1];
        float     fz110=   fz[ctrx1 +ctry1 +ctrz0];
        float     fz111=   fz[ctrx1 +ctry1 +ctrz1];
        float    fxy000=  fxy[ctr];
        float    fxy001=  fxy[ctrx0 +ctry0 +ctrz1];
        float    fxy010=  fxy[ctrx0 +ctry1 +ctrz0];
        float    fxy011=  fxy[ctrx0 +ctry1 +ctrz1];
        float    fxy100=  fxy[ctrx1 +ctry0 +ctrz0];
        float    fxy101=  fxy[ctrx1 +ctry0 +ctrz1];
        float    fxy110=  fxy[ctrx1 +ctry1 +ctrz0];
        float    fxy111=  fxy[ctrx1 +ctry1 +ctrz1];
        float    fxz000=  fxz[ctr];
        float    fxz001=  fxz[ctrx0 +ctry0 +ctrz1];
        float    fxz010=  fxz[ctrx0 +ctry1 +ctrz0];
        float    fxz011=  fxz[ctrx0 +ctry1 +ctrz1];
        float    fxz100=  fxz[ctrx1 +ctry0 +ctrz0];
        float    fxz101=  fxz[ctrx1 +ctry0 +ctrz1];
        float    fxz110=  fxz[ctrx1 +ctry1 +ctrz0];
        float    fxz111=  fxz[ctrx1 +ctry1 +ctrz1];
        float    fyz000=  fyz[ctr];
        float    fyz001=  fyz[ctrx0 +ctry0 +ctrz1];
        float    fyz010=  fyz[ctrx0 +ctry1 +ctrz0];
        float    fyz011=  fyz[ctrx0 +ctry1 +ctrz1];
        float    fyz100=  fyz[ctrx1 +ctry0 +ctrz0];
        float    fyz101=  fyz[ctrx1 +ctry0 +ctrz1];
        float    fyz110=  fyz[ctrx1 +ctry1 +ctrz0];
        float    fyz111=  fyz[ctrx1 +ctry1 +ctrz1];
        float   fxyz000= fxyz[ctr];
        float   fxyz001= fxyz[ctrx0 +ctry0 +ctrz1];
        float   fxyz010= fxyz[ctrx0 +ctry1 +ctrz0];
        float   fxyz011= fxyz[ctrx0 +ctry1 +ctrz1];
        float   fxyz100= fxyz[ctrx1 +ctry0 +ctrz0];
        float   fxyz101= fxyz[ctrx1 +ctry0 +ctrz1];
        float   fxyz110= fxyz[ctrx1 +ctry1 +ctrz0];
        float   fxyz111= fxyz[ctrx1 +ctry1 +ctrz1];
        float s1=     f000-f001-f010+f011;    
        float s2=  s1-f100+f101+f110-f111;    
        float s3=     fx000-fx001-fx010+fx011;
        float s4=  s3+fx100-fx101-fx110+fx111;
        float s5=     fy000-fy001-fy100+fy101;
        float s6=  s5+fy010-fy011-fy110+fy111; 
        float s7=     fz000-fz010-fz100+fz110; 
        float s8=  s7+fz001-fz011-fz101+fz111; 
        float s9=   fxy000-fxy001+fxy010-fxy011+fxy100-fxy101+fxy110-fxy111;
        float s10=  2*s9+2*(fxy000-fxy001)-(fxy110-fxy111);                 
        float s11=  s9+fxy000-fxy001+fxy100-fxy101;                         
        float s12=  s9+fxy000-fxy001+fxy010-fxy011;                        
        float s13=  fxz000+fxz001-fxz010-fxz011+fxz100+fxz101-fxz110-fxz111;
        float s14=  s13-fxz001+fxz011-fxz101+fxz111;                        
        float s15=  s13+fxz000+fxz001-fxz010-fxz011;                        
        float s16=2*s13+2*(fxz000-fxz010)-fxz101+fxz111;                   
        float s17= fyz000+fyz001+fyz010+fyz011-fyz100-fyz101-fyz110-fyz111;
        float s18= fyz000+fyz001-fyz100-fyz101;                            
        float s19= fyz000+fyz010-fyz100-fyz110;                           
        float s20= 2*(fyz000-fyz100)-fyz011+fyz111;   
       //alpha is a N*64 matrix of polynomial coefficients, where N is the number of elements in f
       alpha[ctr     ]=f000;
       alpha[ctr+   N]=fz000;
       alpha[ctr+N*2 ]=(f001-f000)*3-fz001-2*fz000;
       alpha[ctr+N*3 ]=(f000-f001)*2+fz001+fz000;
       alpha[ctr+N*4 ]=fy000;
       alpha[ctr+N*5 ]=fyz000;
       alpha[ctr+N*6 ]=(fy001-fy000)*3-2*fyz000-fyz001;
       alpha[ctr+N*7 ]=(fy000-fy001)*2+fyz000+fyz001;
       alpha[ctr+N*8 ]=(f010-f000)*3-fy010-2*fy000;
       alpha[ctr+N*9 ]=(fz010-fz000)*3-2*fyz000-fyz010;
       alpha[ctr+N*10]=s1*9+(fy000-fz010+fz000-fy001)*6+(fyz010+fyz001)*2+(fy010+fz001-fy011-fz011)*3+4*fyz000+fyz011;
       alpha[ctr+N*11]=(fy001-fy000)*4-s1*6+(fy011-fy010-fyz001-fyz000)*2+(fz010-fz001-fz000+fz011)*3-fyz010-fyz011;
       alpha[ctr+N*12]=(f000-f010)*2+fy010+fy000;
       alpha[ctr+N*13]=(fz000-fz010)*2+fyz000+fyz010;
       alpha[ctr+N*14]=(fz010-fz000)*4-s1*6+(fz011-fyz000-fyz010-fz001)*2+(fy001-fy000+fy011-fy010)*3-fyz001-fyz011;
       alpha[ctr+N*15]=s1*4+(fz001-fz010-fy011+fy010-fy001+fy000+fz000-fz011)*2+fyz000+fyz001+fyz011+fyz010;
       alpha[ctr+N*16]=fx000;
       alpha[ctr+N*17]=fxz000;
       alpha[ctr+N*18]=(fx001-fx000)*3-fxz001-2*fxz000;
       alpha[ctr+N*19]=(fx000-fx001)*2+fxz001+fxz000;
       alpha[ctr+N*20]=fxy000;
       alpha[ctr+N*21]=fxyz000;
       alpha[ctr+N*22]=(fxy001-fxy000)*3-fxyz001-2*fxyz000;
       alpha[ctr+N*23]=(fxy000-fxy001)*2+fxyz001+fxyz000;
       alpha[ctr+N*24]=(fx010-fx000)*3-fxy010-2*fxy000;
       alpha[ctr+N*25]=(fxz010-fxz000)*3-fxyz010-2*fxyz000;
       alpha[ctr+N*26]=s3*9+(fxyz001+fxyz010)*2+(s10-s11+s16-s13-s14)*3+4*fxyz000+fxyz011;
       alpha[ctr+N*27]=(s11-s10-fxyz000-fxyz001)*2-s3*6-(s15-s13)*3-fxyz010-fxyz011;
       alpha[ctr+N*28]=(fx000-fx010)*2+fxy010+fxy000;
       alpha[ctr+N*29]=(fxz000-fxz010)*2+fxyz010+fxyz000;
       alpha[ctr+N*30]=(s13+s14-s16-fxyz000-fxyz010)*2-s3*6-(s12-s9)*3-fxyz001-fxyz011;
       alpha[ctr+N*31]=s3*4+(s12-s9+s15-s13)*2+fxyz000+fxyz011+fxyz010+fxyz001;
       alpha[ctr+N*32]=(f100-f000)*3-fx100-2*fx000;
       alpha[ctr+N*33]=(fz100-fz000)*3-fxz100-2*fxz000;
       alpha[ctr+N*34]=(f000+f101-f100-f001)*9+(fx000-fx001+fz000-fz100)*6+(fxz100+fxz001)*2+(fz001-fz101+fx100-fx101)*3+fxz101+4*fxz000;
       alpha[ctr+N*35]=(f001-f000-f101+f100)*6+(fx001-fx000)*4+(fx101-fx100-fxz001-fxz000)*2+(fz101-fz000+fz100-fz001)*3-fxz100-fxz101;
       alpha[ctr+N*36]=(fy100-fy000)*3-fxy100-2*fxy000;
       alpha[ctr+N*37]=(fyz100-fyz000)*3-fxyz100-2*fxyz000;
       alpha[ctr+N*38]=s5*9+(fxyz100+fxyz001)*2+(s10-s12+fyz000-fyz100+s18)*3+4*fxyz000+fxyz101;
       alpha[ctr+N*39]=(s12-s10-fxyz000-fxyz001)*2-s5*6-s18*3-fxyz101-fxyz100;
       alpha[ctr+N*40]=(f000+f110-f010-f100)*9+(fx000-fy100-fx010+fy000)*6+(fxy100+fxy010)*2+(fx100-fy110+fy010-fx110)*3+fxy110+4*fxy000;
       alpha[ctr+N*41]=9*s7+(fxyz100+fxyz010)*2+(s16-s15+s19+fyz000-fyz100)*3+4*fxyz000+fxyz110;
       alpha[ctr+N*42]=-s2*27-9*(s3+s4+s5+s6+s7+s8)-3*(s10+s16+2*s17+s20)-8*fxyz000-(fxyz101+fxyz110+fxyz011)*2-fxyz111-(fxyz100+fxyz010+fxyz001)*4;
       alpha[ctr+N*43]=s2*18+6*(s3+s4+s5+s6)+s8*9+(fxyz000+fxyz001)*4+(s10+fxyz011+fxyz010+fxyz100+fxyz101)*2+(s15+s17+s18)*3+fxyz111+fxyz110;
       alpha[ctr+N*44]=(f010-f110+f100-f000)*6+(fx010-fx000)*4+(fx110-fxy000-fx100-fxy010)*2+(fy100-fy010-fy000+fy110)*3-fxy110-fxy100;
       alpha[ctr+N*45]=-s7*6-(s16-s15+fxyz010+fxyz000)*2-s19*3-fxyz100-fxyz110;
       alpha[ctr+N*46]=s2*18+6*(s3+s4+s7+s8)+s6*9+(s12+s17+s19)*3+(s16+fxyz011+fxyz100+fxyz001+fxyz110)*2+(fxyz010+fxyz000)*4+fxyz101+fxyz111;
       alpha[ctr+N*47]=-s2*12-4*(s3+s4)-2*(s12+s15)-6*(s6+s8)-fxyz101-fxyz111-fxyz100-s17*3-(fxyz001+fxyz010+fxyz011+fxyz000)*2-fxyz110;
       alpha[ctr+N*48]=(f000-f100)*2+fx100+fx000;
       alpha[ctr+N*49]=(fz000-fz100)*2+fxz100+fxz000;
       alpha[ctr+N*50]=(f001-f000-f101+f100)*6+(fz100-fz000)*4+(fz101-fxz100-fxz000-fz001)*2+(fx001-fx100-fx000+fx101)*3-fxz001-fxz101;
       alpha[ctr+N*51]=(f000+f101-f100-f001)*4+(fx100-fz101-fx001+fx000+fz001+fz000-fz100-fx101)*2+fxz100+fxz001+fxz000+fxz101;
       alpha[ctr+N*52]=(fy000-fy100)*2+fxy100+fxy000;
       alpha[ctr+N*53]=(fyz000-fyz100)*2+fxyz100+fxyz000;
       alpha[ctr+N*54]=(fyz100-fyz000-s18-fxyz000-fxyz100)*2-s5*6-(s11-s9)*3-fxyz101-fxyz001;
       alpha[ctr+N*55]=s5*4+(s11-s9+s18)*2+fxyz000+fxyz101+fxyz100+fxyz001;
       alpha[ctr+N*56]=(f010-f110+f100-f000)*6+(fy100-fy000)*4+(fy110-fxy100-fy010-fxy000)*2+(fx010-fx100-fx000+fx110)*3-fxy110-fxy010;
       alpha[ctr+N*57]=(fyz100-fyz000)*4-s7*6+(fyz110-fyz010-fxyz100-fxyz000)*2-s14*3-fxyz010-fxyz110;
       alpha[ctr+N*58]=s2*18+9*s4+6*(s5+s6+s7+s8)+3*(s11+s13+s14)+fxyz011+(2*s17+s20+fxyz001+fxyz010+fxyz110+fxyz101)*2+fxyz111+(fxyz000+fxyz100)*4;
       alpha[ctr+N*59]=-s2*12-6*(s4+s8)-4*(s5+s6)-3*s13-(s11+s17+s18+fxyz000+fxyz100+fxyz101+fxyz001)*2-fxyz111-fxyz011-fxyz110-fxyz010;
       alpha[ctr+N*60]=(f000+f110-f010-f100)*4+(fx100-fx010+fx000-fy110-fy100+fy010+fy000-fx110)*2+fxy110+fxy100+fxy010+fxy000;
       alpha[ctr+N*61]=s7*4+(s14+s19)*2+fxyz100+fxyz110+fxyz000+fxyz010;
       alpha[ctr+N*62]=-s2*12-6*(s4+s6)-4*(s7+s8)-3*s9-fxyz001-fxyz011-(s13+s14+s17+s19+fxyz110+fxyz010+fxyz100+fxyz000)*2-fxyz101-fxyz111;
       alpha[ctr+N*63]=s2*8+4*(s4+s6+s8)+fxyz001+fxyz010+(s9+s13+s17)*2+fxyz000+fxyz100+fxyz101+fxyz111+fxyz011+fxyz110;
        }}}

#ifdef TIME_COUT
  logfile_printf(" time tricubic_alpha 3 =  %f\n" << double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif

  return ;
}
