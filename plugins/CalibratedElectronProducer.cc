// This file is imported from:

// -*- C++ -*-
//
// Package:    EgammaElectronProducers
// Class:      CalibratedElectronProducer
//
/**\class CalibratedElectronProducer 

 Description: EDProducer of GsfElectron objects

 Implementation:
     <Notes on implementation>
*/




#include "EgammaAnalysis/ElectronTools/plugins/CalibratedElectronProducer.h"
#include "EgammaAnalysis/ElectronTools/interface/EpCombinationTool.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"

#include "DataFormats/EgammaCandidates/interface/GsfElectronFwd.h"
#include "DataFormats/EgammaReco/interface/ElectronSeed.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrack.h"
#include "DataFormats/EgammaCandidates/interface/GsfElectronFwd.h"
#include "Geometry/CaloEventSetup/interface/CaloTopologyRecord.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "EgammaAnalysis/ElectronTools/interface/ElectronEnergyCalibrator.h"
#include "EgammaAnalysis/ElectronTools/interface/SuperClusterHelper.h"

#include <iostream>

using namespace edm ;
using namespace std ;
using namespace reco ;

CalibratedElectronProducer::CalibratedElectronProducer( const edm::ParameterSet & cfg )
// : PatElectronBaseProducer(cfg)
 {


  

  inputElectrons_ = cfg.getParameter<edm::InputTag>("inputElectronsTag");

  nameEnergyReg_      = cfg.getParameter<edm::InputTag>("nameEnergyReg");
  nameEnergyErrorReg_ = cfg.getParameter<edm::InputTag>("nameEnergyErrorReg");

  recHitCollectionEB_ = cfg.getParameter<edm::InputTag>("recHitCollectionEB");
  recHitCollectionEE_ = cfg.getParameter<edm::InputTag>("recHitCollectionEE");


  nameNewEnergyReg_      = cfg.getParameter<std::string>("nameNewEnergyReg");
  nameNewEnergyErrorReg_ = cfg.getParameter<std::string>("nameNewEnergyErrorReg");
  newElectronName_ = cfg.getParameter<std::string>("outputGsfElectronCollectionLabel");


  dataset = cfg.getParameter<std::string>("inputDataset");
  isMC = cfg.getParameter<bool>("isMC");
  updateEnergyError = cfg.getParameter<bool>("updateEnergyError");
  lumiRatio = cfg.getParameter<double>("lumiRatio");
  correctionsType = cfg.getParameter<int>("correctionsType");
  combinationType = cfg.getParameter<int>("combinationType");
  verbose = cfg.getParameter<bool>("verbose");
  synchronization = cfg.getParameter<bool>("synchronization");
  
  //basic checks
  if (isMC&&(dataset!="Summer11"&&dataset!="Fall11"&&dataset!="Summer12"&&dataset!="Summer12_DR53X_HCP2012"))
   { throw cms::Exception("CalibratedgsfElectronProducer|ConfigError")<<"Unknown MC dataset" ; }
  if (!isMC&&(dataset!="Prompt"&&dataset!="ReReco"&&dataset!="Jan16ReReco"&&dataset!="ICHEP2012"&&dataset!="Moriond2013"))
   { throw cms::Exception("CalibratedgsfElectronProducer|ConfigError")<<"Unknown Data dataset" ; }
   cout << "[CalibratedGsfElectronProducer] Correcting scale for dataset " << dataset << endl;


   produces<edm::ValueMap<double> >(nameNewEnergyReg_);
   produces<edm::ValueMap<double> >(nameNewEnergyErrorReg_);
   produces<GsfElectronCollection> (newElectronName_);
   geomInitialized_ = false;
 }
 
CalibratedElectronProducer::~CalibratedElectronProducer()
 {}

void CalibratedElectronProducer::produce( edm::Event & event, const edm::EventSetup & setup )
 {

  if (!geomInitialized_) {
    edm::ESHandle<CaloTopology> theCaloTopology;
    setup.get<CaloTopologyRecord>().get(theCaloTopology);
    ecalTopology_ = & (*theCaloTopology);
    
    edm::ESHandle<CaloGeometry> theCaloGeometry;
    setup.get<CaloGeometryRecord>().get(theCaloGeometry); 
    caloGeometry_ = & (*theCaloGeometry);
    geomInitialized_ = true;
  }

   // Read GsfElectrons
   edm::Handle<reco::GsfElectronCollection>  oldElectronsH ;
   event.getByLabel(inputElectrons_,oldElectronsH) ;
   
   // Read RecHits
  edm::Handle< EcalRecHitCollection > pEBRecHits;
  edm::Handle< EcalRecHitCollection > pEERecHits;
  event.getByLabel( recHitCollectionEB_, pEBRecHits );
  event.getByLabel( recHitCollectionEE_, pEERecHits );

   // ReadValueMaps
  edm::Handle<edm::ValueMap<double> > valMapEnergyH;
  event.getByLabel(nameEnergyReg_,valMapEnergyH);
  edm::Handle<edm::ValueMap<double> > valMapEnergyErrorH;
  event.getByLabel(nameEnergyErrorReg_,valMapEnergyErrorH);
  

  // Prepare output collections
  std::auto_ptr<GsfElectronCollection> electrons( new reco::GsfElectronCollection ) ;
  // Fillers for ValueMaps:
  std::auto_ptr<edm::ValueMap<double> > regrNewEnergyMap(new edm::ValueMap<double>() );
  edm::ValueMap<double>::Filler energyFiller(*regrNewEnergyMap);

  std::auto_ptr<edm::ValueMap<double> > regrNewEnergyErrorMap(new edm::ValueMap<double>() );
  edm::ValueMap<double>::Filler energyErrorFiller(*regrNewEnergyErrorMap);

  // first clone the initial collection
  unsigned nElectrons = oldElectronsH->size();
  for(unsigned iele = 0; iele < nElectrons; ++iele){    
    electrons->push_back((*oldElectronsH)[iele]);
  }

  std::vector<double> regressionValues;
  std::vector<double> regressionErrorValues;

  std::string pathToDataCorr;
  if (correctionsType != 0 ){

  switch (correctionsType){

	  case 1: pathToDataCorr = "../data/scalesMoriond.csv"; 
		  if (verbose) {std::cout<<"You choose regression 1 scale corrections"<<std::endl;}
		  break;
	  case 2: throw cms::Exception("CalibratedgsfElectronProducer|ConfigError")<<"You choose regression 2 scale corrections. They are not implemented yet." ;
		 // pathToDataCorr = "../data/data.csv";
		 // if (verbose) {std::cout<<"You choose regression 2 scale corrections."<<std::endl;}
		  break;
	  case 3: pathToDataCorr = "../data/data.csv";
		  if (verbose) {std::cout<<"You choose standard ecal energy scale corrections"<<std::endl;}
		  break;
	  default: throw cms::Exception("CalibratedgsfElectronProducer|ConfigError")<<"Unknown correctionsType !!!" ;
  }

  ElectronEnergyCalibrator theEnCorrector(pathToDataCorr, dataset, correctionsType, lumiRatio, isMC, updateEnergyError, verbose, synchronization);

  if (verbose) {std::cout<<"ElectronEnergyCalibrator object is created "<<std::endl;}

  for ( unsigned iele = 0; iele < nElectrons ; ++iele) {

    reco::GsfElectron & ele  ( (*electrons)[iele]);
    reco::GsfElectronRef elecRef(oldElectronsH,iele);
    double regressionEnergy = (*valMapEnergyH)[elecRef];
    double regressionEnergyError = (*valMapEnergyErrorH)[elecRef];
    
    regressionValues.push_back(regressionEnergy);
    regressionErrorValues.push_back(regressionEnergyError);

    //    r9 
    const EcalRecHitCollection * recHits=0;
    if(ele.isEB()) {
      recHits = pEBRecHits.product();
    } else
      recHits = pEERecHits.product();

    SuperClusterHelper mySCHelper(&(ele),recHits,ecalTopology_,caloGeometry_);

    int elClass = -1;
    int run = event.run(); 

    float r9 = mySCHelper.r9(); 
    double correctedEcalEnergy = ele.correctedEcalEnergy();
    double correctedEcalEnergyError = ele.correctedEcalEnergyError();
    double trackMomentum = ele.trackMomentumAtVtx().R();
    double trackMomentumError = ele.trackMomentumError();
    
    if (ele.classification() == reco::GsfElectron::GOLDEN) {elClass = 0;}
    if (ele.classification() == reco::GsfElectron::BIGBREM) {elClass = 1;}
    if (ele.classification() == reco::GsfElectron::BADTRACK) {elClass = 2;}
    if (ele.classification() == reco::GsfElectron::SHOWERING) {elClass = 3;}
    if (ele.classification() == reco::GsfElectron::GAP) {elClass = 4;}

    SimpleElectron mySimpleElectron(run, elClass, r9, correctedEcalEnergy, correctedEcalEnergyError, trackMomentum, trackMomentumError, regressionEnergy, regressionEnergyError, ele.superCluster()->eta(), ele.isEB(), isMC, ele.ecalDriven(), ele.trackerDrivenSeed());

    // energy calibration for ecalDriven electrons
      if (ele.core()->ecalDrivenSeed()) {        
        theEnCorrector.calibrate(mySimpleElectron);
      }
    // E-p combination  
      ElectronEPcombinator myCombinator;
      EpCombinationTool MyEpCombinationTool;
      MyEpCombinationTool.init("../data/GBR_EGclusters_PlusPshw_Pt5-300_weighted_pt_5-300_Cut50_PtSlope50_Significance_5_results.root","CombinationWeight");
      float regCombMomentum = mySimpleElectron.getRegEnergy();
      float regCombMomentumError =mySimpleElectron.getRegEnergyError();

      switch (combinationType){
	  case 0: 
		  if (verbose) {std::cout<<"You choose not to combine."<<std::endl;}
		  break;
	  case 1: 
		  if (verbose) {std::cout<<"You choose corrected regression energy for standard combination"<<std::endl;}
		  myCombinator.setCombinationMode(1);
		  myCombinator.combine(mySimpleElectron);
		  break;
	  case 2: 
		  if (verbose) {std::cout<<"You choose uncorrected regression energy for standard combination"<<std::endl;}
		  myCombinator.setCombinationMode(2);
		  myCombinator.combine(mySimpleElectron);
		  break;
	  case 3: 
		  if (verbose) {std::cout<<"You choose regression combination."<<std::endl;}
		  MyEpCombinationTool.combine(mySimpleElectron);
		  break;
	  default: 
		  throw cms::Exception("CalibratedgsfElectronProducer|ConfigError")<<"Unknown combination Type !!!" ;
      }


  math::XYZTLorentzVector oldMomentum = ele.p4() ;
  math::XYZTLorentzVector newMomentum_ ;
  newMomentum_ = math::XYZTLorentzVector
   ( oldMomentum.x()*mySimpleElectron.getCombinedMomentum()/oldMomentum.t(),
     oldMomentum.y()*mySimpleElectron.getCombinedMomentum()/oldMomentum.t(),
     oldMomentum.z()*mySimpleElectron.getCombinedMomentum()/oldMomentum.t(),
     mySimpleElectron.getCombinedMomentum() ) ;

  if (verbose) {std::cout<<"Combined momentum before saving  "<<ele.p4().t()<<std::endl;}
  if (verbose) {std::cout<<"Combined momentum FOR saving  "<<mySimpleElectron.getCombinedMomentum()<<std::endl;}

  ele.correctMomentum(newMomentum_,mySimpleElectron.getTrackerMomentumError(),mySimpleElectron.getCombinedMomentumError());

  if (verbose) {std::cout<<"Combined momentum after saving  "<<ele.p4().t()<<std::endl;}


   }
  } else {std::cout<<"You choose not to correct. Uncorrected Regression Energy is taken."<<std::endl;}

  
  // Save the electrons
  const edm::OrphanHandle<reco::GsfElectronCollection> gsfNewElectronHandle = event.put(electrons, newElectronName_) ;
  energyFiller.insert(gsfNewElectronHandle,regressionValues.begin(),regressionValues.end());
  energyFiller.fill();
  energyErrorFiller.insert(gsfNewElectronHandle,regressionErrorValues.begin(),regressionErrorValues.end());
  energyErrorFiller.fill();


  event.put(regrNewEnergyMap,nameNewEnergyReg_);
  event.put(regrNewEnergyErrorMap,nameNewEnergyErrorReg_);
 }


#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/ESProducer.h"
#include "FWCore/Framework/interface/ModuleFactory.h"
DEFINE_FWK_MODULE(CalibratedElectronProducer);

//#endif
