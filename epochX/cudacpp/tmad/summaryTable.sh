#!/bin/sh

# Include CUDA/8tpb?
cuda8tpb=
###cuda8tpb="CUDA/8tpb"

table=
if [ "$1" == "-ALL" ] && [ "$2" == "" ]; then
  set -e
  $0 -default
  $0 -juwels
  exit 0
elif [ "$1" == "-default" ]; then
  table="default"; shift
elif [ "$1" == "-juwels" ]; then
  table="juwels"; shift
else
  echo "Usage: $0 <table [-ALL|-default|-juwels]>"; exit 1
fi

# Select revisions of mad logs
mrevs=""
if [ "$table" == "default" ]; then
  mrevs="$mrevs deb4c9b"  # cuda116/gcc102  (22 Jun 2022)
elif [ "$table" == "juwels" ]; then
  mrevs="$mrevs df94bdf"  # cuda116/gcc102  (22 Jun 2022) 
fi
revs="$mrevs"
  
# Kernel function
function oneTable()
{
  taglist="FORTRAN CPP/none CPP/sse4 CPP/avx2 CPP/512y CPP/512z CUDA/32 CUDA/8192 CUDA/max $cuda8tpb"
  parlist="(1) (2-none) (2-sse4) (2-avx2) (2-512y) (2-512z) (3) (3bis)"
  faclist="1 10 100"
  for proc in $procs; do
    file=tmad/logs_${proc}_${suff}/log_${proc}_${suff}_${fpt}_${inl}_${hrd}.txt
    if [ ! -f $file ]; then continue; fi
    ###echo "*** FILE $file ***"
    git checkout $rev $file >& /dev/null
    if [ "$?" != "0" ]; then echo "ERROR! 'git checkout $rev' failed!"; exit 1; fi
    node=$(cat $file | grep ^On | sort -u)
    if [ "$nodelast" != "$node" ]; then echo -e "$node\n" >> $out; nodelast=$node; fi
    cat $file | awk -vproc=$proc -vtaglist="$taglist" -vparlist="$parlist" -vfaclist="$faclist" '
      BEGIN{status=0;}
      BEGIN{ntag=split(taglist,tags); npar=split(parlist,pars);
            ###if(ntag!=npar){print "ERROR! ntag!=npar", ntag, npar; status=1; exit status}; # NB new ntag>npar!
            for(i=1;i<=npar;i++){tag1[pars[i]]=tags[i];}}
      BEGIN{nfac=split(faclist,facs)}
      BEGIN{lsepEQUAL=sprintf("%0128d",0); lsepDASH=lsepEQUAL; gsub("0","-",lsepDASH); gsub("0","=",lsepEQUAL)}
      BEGIN{lsepEQUAL2=sprintf("%014d%91s%023d",0,"",0); lsepDASH2=lsepEQUAL2; gsub("0","-",lsepDASH2); gsub("0","=",lsepEQUAL2)}
      ###/create events.lhe/{print $0}
      /create events.lhe/{par=$2; tag=tag1[par]} # current tag (FORTRAN... CUDA/8192)
      /GCHECK\(MAX\)/{tag="CUDA/max"} # current tag (CUDA/max)
      /GCHECK\(MAX128THR\)/{tag="CUDA/max128t"} # current tag (CUDA/max128t)
      /GCHECK\(MAX8THR\)/{tag="CUDA/8tpb"} # current tag (CUDA/8tpb)
      /create events.lhe/{fac=substr($5,2)} # current fac
      /\[COUNTERS\]/{if($3=="TOTAL") typ=1; else if($3=="Overhead") typ=2; else if($3=="MEs") typ=3;
                     else{print "ERROR! Unknown type $3"; status=1; exit status};
                     if($4==":") sec=$5; else sec=$8; sec=substr(sec,1,length(sec)-1); # current sec
                     sec3[tag,fac,typ]=sec}
      /\[COUNTERS\]/{if(tag!="" && $3=="MEs")
                     {nevt=$10; ###print tag, nevt;
                      if(tag=="FORTRAN") nevt1[fac]=nevt; 
                      else if(tag=="CUDA/8192") nevt1b[fac]=nevt;
                      else if(nevt1[fac]!=nevt){print "ERROR! nevt mismatch", nevt1[fac], nevt; status=1; exit status};
                      tputm1[tag]=tolower($15)}}
      /CHECK/{if(tag!=""){if($5=="2048") fld5="2k";
                          else if($5=="16384") fld5="16k";
                          else if($5=="65536") fld5="64k";
                          else fld5=$5;
                          if($8=="--bridge"){gcheck="bridge"; sabg1[tag]=$5*$6; sabp1[tag]=fld5"*"$6"*"$7}
                          else{gcheck="nobridge"; sag1[tag]=$5*$6; sap1[tag]=fld5"*"$6"*"$7}}}
      ###/CHECK/{print $0}
      ###/CHECK/{print tag, sabg1[tag], sabp1[tag], sag1[tag], sap[tag]; }
      /EvtsPerSec/{if(tag!="" && gcheck=="bridge"){tputb1[tag]=$5}}
      /EvtsPerSec/{if(tag!="" && gcheck!="bridge"){tput1[tag]=$5}}
      END{if (status!=0) exit status;
          print lsepEQUAL;
          printf "| %-10s | %-78s | %-30s |\n", proc, "[sec] Total = Overhead (FORTRAN) + MEs (FORTRAN, CPP or CUDA)", "[MEs/sec]"; 
          print lsepDASH;
          printf "| %-10s | %24s | %24s | %35s | %8s | %8s |\n", "", "mad", "mad", "mad", "sa/brdg", "sa/full";
          print lsepEQUAL;
          for (itag=1; itag<=ntag; itag++)
          {tag=tags[itag]; 
           if(tag=="FORTRAN"){printf "| %-10s | %24s | %24s | %35s | %8s | %8s |\n",
                              "nevt/grid", "32", "32", "32", sabg1["CUDA/32"], sag1["CUDA/32"];
                              printf "| %-10s | %24s | %24s | %35s | %8s | %8s |\n",
                              "nevt total", "x"facs[1]" ["nevt1[facs[1]]"]", "x"facs[2]" ["nevt1[facs[2]]"]", "x"facs[3]" ["nevt1[facs[3]]"]",
                              sabp1["CUDA/32"], sap1["CUDA/32"];
                              print lsepDASH}
           else if(tag=="CUDA/8192"){
                              print lsepEQUAL;
                              printf "| %-10s | %24s | %24s | %35s | %8s | %8s |\n",
                              "nevt/grid", "8192", "8192", "8192", sabg1[tag], sag1[tag];
                              printf "| %-10s | %24s | %24s | %35s | %8s | %8s |\n",
                              "nevt total", "x"facs[1]" ["nevt1b[facs[1]]"]", "x"facs[2]" ["nevt1b[facs[2]]"]", "x"facs[3]" ["nevt1b[facs[3]]"]",
                              sabp1[tag], sap1[tag];
                              print lsepDASH}
           else if(tag=="CUDA/max"||tag=="CUDA/8tpb"){
                              if(tag=="CUDA/max") print lsepEQUAL; else print lsepEQUAL2;
                              printf "| %-10s | %89s | %8s | %8s |\n",
                              "nevt/grid", "", sabg1[tag], sag1[tag];
                              printf "| %-10s | %89s | %8s | %8s |\n",
                              "nevt total", "", sabp1[tag], sap1[tag];
                              print lsepDASH2};
           printf "| %-10s |", tag;
           if(tag=="CUDA/max"||tag=="CUDA/8tpb"){ printf " %89s |", ""; }
           else{ for(ifac=1; ifac<=nfac; ifac++)
                 { fac=facs[ifac]; printf " %6.2f = %6.2f + %6.2f |", sec3[tag,fac,1], sec3[tag,fac,2], sec3[tag,fac,3]};
                 printf " %8s |", tputm1[tag]; }
           if(tag=="FORTRAN"){ printf " %8s | %8s |", "---", "---"; }
           else{ printf " %8.2e |", tputb1[tag]; printf " %8.2e |", tput1[tag]; }
           printf "\n"};
          print lsepEQUAL2;
          print "\n";
         }' >> $out
  done
}

cd $(dirname $0)/..
echo PWD=$(pwd)

suff=mad
fpt=d
inl=inl0
hrd=hrd0

procs="eemumu ggtt ggttg ggttgg ggttggg"
###procs="ggttggg"

out=tmad/summaryTable_${table}.txt
echo "" > $out
for rev in $revs; do
  echo -e "+++ $bckend REVISION $rev (commit date: $(git log $rev --pretty=format:'%ci' --abbrev-commit -n1)) +++" >> $out
  oneTable
done

git checkout HEAD ../cudacpp/tmad/logs* >& /dev/null
cat $out
