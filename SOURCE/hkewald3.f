      subroutine hkewald3
     x  (iatm,ik,engcpe,vircpe,epsq,nexatm,lexatm,
     x  chge,xdf,ydf,zdf,fxx,fyy,fzz,stress)  
c     
c***********************************************************************
c     
c     dl_poly subroutine for calculating exclusion corrections to
c     the hautman-klein-ewald electrostatic method
c     
c     parallel replicated data version
c     
c     copyright - daresbury laboratory 2000
c     author    - w. smith  may 2000
c     
c     wl
c     2001/06/12 13:10:09
c     1.1
c     Exp
c     
c***********************************************************************
c     
      
#include "dl_params.inc"
      
      dimension nexatm(msatms),lexatm(msatms,mxexcl)
      dimension xdf(mxxdf),ydf(mxxdf),zdf(mxxdf)
      dimension fxx(mxatms),fyy(mxatms),fzz(mxatms)
      dimension chge(mxatms),stress(9)

CDIR$ CACHE_ALIGN fi
#ifdef VAMPIR
      call VTBEGIN(173, ierr)
#endif
#ifdef STRESS
c     
c     initialise stress tensor accumulators
      strs1 = 0.d0
      strs2 = 0.d0
      strs3 = 0.d0
      strs5 = 0.d0
      strs6 = 0.d0
      strs9 = 0.d0
#endif
c     
c     initialise potential energy and virial
      
      engcpe=0.d0
      vircpe=0.d0     
c     
c     start of primary loop for forces evaluation
      
      chgea = chge(iatm)/epsq*r4pie0
      
      if(chgea.ne.0.d0) then
        
        do m=1,nexatm(ik)
c     
c     atomic index and charge product
          
          jatm=lexatm(ik,m)
          chgprd=chgea*chge(jatm)
          
          if(chgprd.ne.0.d0) then
c     
c     calculate interatomic distance
            
            rsq=xdf(m)**2+ydf(m)**2+zdf(m)**2
            rrr = sqrt(rsq)
c     
c     calculate potential energy and virial
            
            coul = chgprd/rrr
            engcpe = engcpe - coul
            
c     
c     calculate forces
            
            fcoul = coul/rsq
            fx = fcoul*xdf(m)
            fy = fcoul*ydf(m)
            fz = fcoul*zdf(m)
            
            fxx(iatm) = fxx(iatm) - fx
            fyy(iatm) = fyy(iatm) - fy
            fzz(iatm) = fzz(iatm) - fz

            fxx(jatm) = fxx(jatm) + fx
            fyy(jatm) = fyy(jatm) + fy
            fzz(jatm) = fzz(jatm) + fz
#ifdef STRESS
c     
c     calculate stress tensor
            
            strs1 = strs1 - xdf(m)*fx
            strs2 = strs2 - xdf(m)*fy
            strs3 = strs3 - xdf(m)*fz
            
            strs5 = strs5 - ydf(m)*fy
            strs6 = strs6 - ydf(m)*fz
            
            strs9 = strs9 - zdf(m)*fz
#endif
            
          endif
          
        enddo
c     
c     virial
        
        vircpe = -engcpe
#ifdef STRESS
c     
c     complete stress tensor
        
        stress(1) = stress(1) + strs1
        stress(2) = stress(2) + strs2
        stress(3) = stress(3) + strs3
        stress(4) = stress(4) + strs2
        stress(5) = stress(5) + strs5
        stress(6) = stress(6) + strs6
        stress(7) = stress(7) + strs3
        stress(8) = stress(8) + strs6
        stress(9) = stress(9) + strs9
#endif
      endif
      
#ifdef VAMPIR
      call VTEND(173, ierr)
#endif
      return
      end
