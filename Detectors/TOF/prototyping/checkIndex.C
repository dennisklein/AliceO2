void checkIndex(){
  Int_t indextof[5];
  Int_t chan;
  Int_t indextof2[5];
  Int_t i,j,k,l,m;

  Bool_t ErrorSe=0;
  Bool_t ErrorPl=0;
  Bool_t ErrorSt=0;
  Bool_t ErrorPx=0;
  Bool_t ErrorPz=0;

  for(Int_t i=0;i<18;i++){
    indextof[0] = i;
    for(Int_t j=0;j<5;j++){
      indextof[1] = j;
      for(Int_t k=0;k<19;k++){
	if(j==2 && (k > 14 || i==15 || i==14 || i==13)) continue;
	indextof[2] = k;
	
	Bool_t locError = 0;

	for(Int_t l=0;l<2;l++){
	  indextof[3] = l;
	  for(Int_t m=0;m<48;m++){
	    indextof[4] = m;

	    chan =  o2::tof::Geo::getIndex(indextof);
	    o2::tof::Geo::getVolumeIndices(chan,indextof2);

	    if(indextof[0] != indextof2[0]) ErrorSe=1,locError=1;
	    if(indextof[1] != indextof2[1]) ErrorPl=1,locError=1;
	    if(indextof[2] != indextof2[2]) ErrorSt=1,locError=1;
	    if(indextof[3] != indextof2[3]) ErrorPx=1,locError=1;
	    if(indextof[4] != indextof2[4]) ErrorPz=1,locError=1;


	    if(locError && j==1 && k==3){
	      printf("in:%i %i %i %i %i --> out:%i %i %i %i %i (ch=%i)\n",indextof[0],indextof[1],indextof[2],indextof[3],indextof[4],indextof2[0],indextof2[1],indextof2[2],indextof2[3],indextof2[4],chan);
	    }
	  }
	}
      }
    }
  }

  if(ErrorSe || ErrorPl || ErrorSt || ErrorPx || ErrorPz)
    printf("Mapping errors -> %i %i %i %i %i\n",ErrorSe,ErrorPl,ErrorSt,ErrorPx,ErrorPz);
  else
    printf("OK\n");
}
