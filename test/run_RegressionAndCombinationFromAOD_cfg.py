import FWCore.ParameterSet.Config as cms

process = cms.Process("ExREG")
process.load("Configuration.StandardSequences.Services_cff")
process.load('Configuration.StandardSequences.GeometryDB_cff')
process.load('Configuration/StandardSequences/MagneticField_38T_cff')
process.load('Configuration/StandardSequences/FrontierConditions_GlobalTag_cff')
process.load("Configuration.StandardSequences.Reconstruction_cff")
process.GlobalTag.globaltag = 'START44_V7::All'

process.RandomNumberGeneratorService = cms.Service("RandomNumberGeneratorService",
    calibratedElectrons = cms.PSet(
        initialSeed = cms.untracked.uint32(1),
        engineName = cms.untracked.string('TRandom3')
    ),
)

process.load("EgammaAnalysis.ElectronTools.calibratedElectrons_cfi")

# dataset to correct
process.calibratedElectrons.isMC = cms.bool(True)
process.calibratedElectrons.inputDataset = cms.string("Fall11")
process.calibratedElectrons.updateEnergyError = cms.bool(True)
process.calibratedElectrons.applyCorrections = cms.int32(10)
process.calibratedElectrons.debug = cms.bool(True)

process.load('RecoJets.Configuration.RecoPFJets_cff')
process.kt6PFJets = process.kt6PFJets.clone( Rho_EtaMax = cms.double(2.5), Ghost_EtaMax = cms.double(2.5) )


process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(1000)
    )


process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
'root://pcmssd12//data/gpetrucc/7TeV/hzz/aod/HToZZTo4L_M-120_Fall11S6.00215E21D5C4.root'
    ))


process.load('EgammaAnalysis.ElectronTools.electronRegressionEnergyProducer_cfi')
process.eleRegressionEnergy.inputElectronsTag = cms.InputTag('gsfElectrons')
process.eleRegressionEnergy.inputCollectionType = cms.uint32(0)
process.eleRegressionEnergy.useRecHitCollections = cms.bool(True)
process.eleRegressionEnergy.produceValueMaps = cms.bool(True)
process.eleRegressionEnergy.rhoCollection = cms.InputTag('kt6PFJets:rho')

process.out = cms.OutputModule("PoolOutputModule",
                               outputCommands = cms.untracked.vstring('drop *',
                                                                      'keep *_*_*_ExREG'),
                               fileName = cms.untracked.string('electrons-AOD.root')
                                                              )
process.load("FWCore.MessageLogger.MessageLogger_cfi")

process.p = cms.Path(process.kt6PFJets * process.eleRegressionEnergy * process.calibratedElectrons)
#process.p = cms.Path(process.eleRegressionEnergy )
process.outpath = cms.EndPath(process.out)

