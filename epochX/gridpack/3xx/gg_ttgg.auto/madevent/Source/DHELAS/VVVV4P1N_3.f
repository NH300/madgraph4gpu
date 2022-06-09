C     This File is Automatically generated by ALOHA 
C     The process calculated in this file is: 
C     Metric(1,3)*Metric(2,4) - Metric(1,2)*Metric(3,4)
C     
      SUBROUTINE VVVV4P1N_3(V1, V2, V4, COUP,V3)
      IMPLICIT NONE
      COMPLEX*16 CI
      PARAMETER (CI=(0D0,1D0))
      COMPLEX*16 COUP
      COMPLEX*16 TMP1
      COMPLEX*16 TMP11
      COMPLEX*16 V1(*)
      COMPLEX*16 V2(*)
      COMPLEX*16 V3(6)
      COMPLEX*16 V4(*)
      TMP1 = (V2(3)*V1(3)-V2(4)*V1(4)-V2(5)*V1(5)-V2(6)*V1(6))
      TMP11 = (V2(3)*V4(3)-V2(4)*V4(4)-V2(5)*V4(5)-V2(6)*V4(6))
      V3(3)= COUP*(-CI*(V1(3)*TMP11)+CI*(TMP1*V4(3)))
      V3(4)= COUP*(+CI*(V1(4)*TMP11)-CI*(TMP1*V4(4)))
      V3(5)= COUP*(+CI*(V1(5)*TMP11)-CI*(TMP1*V4(5)))
      V3(6)= COUP*(+CI*(V1(6)*TMP11)-CI*(TMP1*V4(6)))
      END


