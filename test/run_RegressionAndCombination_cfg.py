import FWCore.ParameterSet.Config as cms

process = cms.Process("ExREG")
process.load("Configuration.StandardSequences.Services_cff")
process.load('Configuration.StandardSequences.Geometry_cff')
process.load('Configuration/StandardSequences/MagneticField_38T_cff')
process.load('Configuration/StandardSequences/FrontierConditions_GlobalTag_cff')
process.load("Configuration.StandardSequences.Reconstruction_cff")
process.GlobalTag.globaltag = 'START44_V7::All'

process.RandomNumberGeneratorService = cms.Service("RandomNumberGeneratorService",
                                                   calibratedPatElectrons = cms.PSet(
    initialSeed = cms.untracked.uint32(1),
    engineName = cms.untracked.string('TRandom3')
    ),
                                                   )

process.load("EgammaAnalysis.ElectronTools.calibratedPatElectrons_cfi")

# dataset to correct
process.calibratedPatElectrons.inputDataset = cms.string("Fall11")
process.calibratedPatElectrons.isMC = cms.bool(True)
process.calibratedPatElectrons.updateEnergyError = cms.bool(True)
process.calibratedPatElectrons.applyCorrections = cms.int32(10)
process.calibratedPatElectrons.debug = cms.bool(True)


process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(1000)
            )


process.source = cms.Source("PoolSource",
                            fileNames = cms.untracked.vstring('file:/localscratch/b/beaudett/egamma/official/CMSSW_4_4_5/src/PhysicsTools/PatAlgos/test/patTuple.root')
                            )


process.load('EgammaAnalysis.ElectronTools.electronRegressionEnergyProducer_cfi')
process.eleRegressionEnergy.rhoCollection = cms.InputTag('kt6PFJets:rho')

process.out = cms.OutputModule("PoolOutputModule",
                               outputCommands = cms.untracked.vstring('drop *',
                                                                      'keep *_*_*_ExREG'),
                               fileName = cms.untracked.string('electrons.root')
                               )
process.load("FWCore.MessageLogger.MessageLogger_cfi")

process.p = cms.Path(process.eleRegressionEnergy * process.calibratedPatElectrons)
process.outpath = cms.EndPath(process.out)
