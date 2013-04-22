#include "EgammaAnalysis/ElectronTools/interface/ElectronEPcombinator.h"
#include <iostream>

//Accessor to the combination results
void ElectronEPcombinator::combine(SimpleElectron & electron){

	electron_ = electron;
	computeEPcombination();
	electron.setCombinedMomentum(combinedMomentum_);
	electron.setCombinedMomentumError(combinedMomentumError_);
}

//Core code to compute the EP combination
void ElectronEPcombinator::computeEPcombination(){

  if (mode_==1){
  scEnergy_ = electron_.getNewEnergy();
  scEnergyError_ = electron_.getNewEnergyError();
  }
  if (mode_==2){
  scEnergy_ = electron_.getRegEnergy();
  scEnergyError_ = electron_.getRegEnergyError();
  }
  trackerMomentum_ = electron_.getTrackerMomentum();
  trackerMomentumError_ = electron_.getTrackerMomentumError();
  elClass_ = electron_.getElClass();

  combinedMomentum_ = scEnergy_; // initial
  combinedMomentumError_ = 999.;
  
  // first check for large errors
 
  if (trackerMomentumError_/trackerMomentum_ > 0.5 && scEnergyError_/scEnergy_ <= 0.5) {
    combinedMomentum_ = scEnergy_;    combinedMomentumError_ = scEnergyError_;
   }
  else if (trackerMomentumError_/trackerMomentum_ <= 0.5 && scEnergyError_/scEnergy_ > 0.5){
    combinedMomentum_ = trackerMomentum_;  combinedMomentumError_ = trackerMomentumError_;
   }
  else if (trackerMomentumError_/trackerMomentum_ > 0.5 && scEnergyError_/scEnergy_ > 0.5){
    if (trackerMomentumError_/trackerMomentum_ < scEnergyError_/scEnergy_) {
      combinedMomentum_ = trackerMomentum_; combinedMomentumError_ = trackerMomentumError_;
     }
    else{
      combinedMomentum_ = scEnergy_; combinedMomentumError_ = scEnergyError_;
     }
  }
  
  // then apply the combination algorithm
  else {

     // calculate E/p and corresponding error
    float eOverP = scEnergy_ / trackerMomentum_;
    float errorEOverP = sqrt(
			     (scEnergyError_/trackerMomentum_)*(scEnergyError_/trackerMomentum_) +
			     (scEnergy_*trackerMomentumError_/trackerMomentum_/trackerMomentum_)*
			     (scEnergy_*trackerMomentumError_/trackerMomentum_/trackerMomentum_));

    //old combination
     if ( eOverP  > 1 + 2.5*errorEOverP )
       {
 	combinedMomentum_ = scEnergy_; combinedMomentumError_ = scEnergyError_;
 	if ((elClass_==0) && electron_.isEB() && (eOverP<1.15))
 	  {
 	    if (scEnergy_<15) {combinedMomentum_ = trackerMomentum_ ; combinedMomentumError_ = trackerMomentumError_;}
 	  }
       }
     else if ( eOverP < 1 - 2.5*errorEOverP )
       {
 	combinedMomentum_ = scEnergy_; combinedMomentumError_ = scEnergyError_;
 	if (elClass_==3)
 	  {
 	    if (electron_.isEB())
 	      {
 		if(scEnergy_<18) {combinedMomentum_ = trackerMomentum_; combinedMomentumError_ = trackerMomentumError_;}
 	      }
 	    else 
 	      {
 		if(scEnergy_<13) {combinedMomentum_ = trackerMomentum_; combinedMomentumError_ = trackerMomentumError_;}
 	      }
 	  }
 	else if (elClass_==4)
 	  {
 	    if(scEnergy_<60) {combinedMomentum_ = trackerMomentum_; combinedMomentumError_ = trackerMomentumError_;}
 	  }
       }
 
     else
      {
       // combination
       combinedMomentum_ = (scEnergy_/scEnergyError_/scEnergyError_ + trackerMomentum_/trackerMomentumError_/trackerMomentumError_) /
         (1/scEnergyError_/scEnergyError_ + 1/trackerMomentumError_/trackerMomentumError_);
       float combinedMomentum_Variance = 1 / (1/scEnergyError_/scEnergyError_ + 1/trackerMomentumError_/trackerMomentumError_);
       combinedMomentumError_ = sqrt(combinedMomentum_Variance);
      }
  } 

}
