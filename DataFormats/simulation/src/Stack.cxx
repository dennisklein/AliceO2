// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Stack.cxx
/// \brief Implementation of the Stack class
/// \author M. Al-Turany, S. Wenzel - June 2014

#include "DetectorsBase/DetID.h"
#include "DetectorsBase/Detector.h"
#include "SimulationDataFormat/Stack.h"
#include "SimulationDataFormat/MCTrack.h"

#include "FairDetector.h"     // for FairDetector
#include "FairLogger.h"       // for MESSAGE_ORIGIN, FairLogger
#include "SimulationDataFormat/BaseHits.h"
#include "FairGenericRootManager.h"  // for FairGenericRootManager
#include "FairRootManager.h"

#include "TClonesArray.h"     // for TClonesArray
#include "TIterator.h"        // for TIterator
#include "TLorentzVector.h"   // for TLorentzVector
#include "TParticle.h"        // for TParticle
#include "TRefArray.h"        // for TRefArray

#include <cstddef>           // for NULL

using std::cout;
using std::endl;
using std::pair;
using namespace o2::Data;

Stack::Stack(Int_t size)
  : FairGenericStack(),
    mStack(),
    mParticles(new TClonesArray("TParticle", size)),
    mTracks(new std::vector<o2::MCTrack>),
    mStoreMap(),
    mStoreIterator(),
    mIndexMap(),
    mIndexIterator(),
    mPointsMap(),
    mIndexOfCurrentTrack(-1),
    mNumberOfPrimaryParticles(0),
    mNumberOfEntriesInParticles(0),
    mNumberOfEntriesInTracks(0),
    mIndex(0),
    mStoreMothers(kTRUE),
    mStoreSecondaries(kTRUE),
    mMinPoints(1),
    mEnergyCut(0.),
    mLogger(FairLogger::GetLogger())
{
  // LOG(INFO) << "Stack::Stack(Int_t) " << this << " mTracks " << mTracks << std::endl;
}

Stack::Stack(const Stack &rhs)
  : FairGenericStack(rhs),
    mStack(),
    mParticles(nullptr),
    mTracks(nullptr),
    mStoreMap(),
    mStoreIterator(),
    mIndexMap(),
    mIndexVector(),
    mIndexIterator(),
    mPointsMap(),
    mIndexOfCurrentTrack(-1),
    mNumberOfPrimaryParticles(0),
    mNumberOfEntriesInParticles(0),
    mNumberOfEntriesInTracks(0),
    mIndex(0),
    mStoreMothers(rhs.mStoreMothers),
    mStoreSecondaries(rhs.mStoreSecondaries),
    mMinPoints(rhs.mMinPoints),
    mEnergyCut(rhs.mEnergyCut),
    mLogger(FairLogger::GetLogger())
{
  mParticles = new TClonesArray("TParticle", rhs.mParticles->GetSize());
  mTracks = new std::vector<MCTrack>(rhs.mTracks->size());

  // LOG(INFO) << "Stack::Stack(rhs) " << this << " mTracks " << mTracks << std::endl;
}

Stack::~Stack()
{
  if (mParticles) {
    mParticles->Delete();
    delete mParticles;
  }
  if (mTracks) {
    delete mTracks;
  }
}

Stack &Stack::operator=(const Stack &rhs)
{
  // check assignment to self
  if (this == &rhs) { return *this; }

  // base class assignment
  FairGenericStack::operator=(rhs);

  // assignment operator
  mParticles = new TClonesArray("TParticle", rhs.mParticles->GetSize());
  mTracks = new std::vector<MCTrack>(rhs.mTracks->size());
  mIndexOfCurrentTrack = -1;
  mNumberOfPrimaryParticles = 0;
  mNumberOfEntriesInParticles = 0;
  mNumberOfEntriesInTracks = 0;
  mIndex = 0;
  mStoreMothers = rhs.mStoreMothers;
  mStoreSecondaries = rhs.mStoreSecondaries;
  mMinPoints = rhs.mMinPoints;
  mEnergyCut = rhs.mEnergyCut;
  mLogger = nullptr;

  return *this;
}

void Stack::PushTrack(Int_t toBeDone, Int_t parentId, Int_t pdgCode, Double_t px, Double_t py, Double_t pz, Double_t e,
                      Double_t vx, Double_t vy, Double_t vz, Double_t time, Double_t polx, Double_t poly, Double_t polz,
                      TMCProcess proc, Int_t &ntr, Double_t weight, Int_t is)
{

  PushTrack(toBeDone, parentId, pdgCode, px, py, pz, e, vx, vy, vz, time, polx, poly, polz, proc, ntr, weight, is, -1);
}

void Stack::PushTrack(Int_t toBeDone, Int_t parentId, Int_t pdgCode, Double_t px, Double_t py, Double_t pz, Double_t e,
                      Double_t vx, Double_t vy, Double_t vz, Double_t time, Double_t polx, Double_t poly, Double_t polz,
                      TMCProcess proc, Int_t &ntr, Double_t weight, Int_t is, Int_t secondparentID)
{

  // Get TParticle array
  TClonesArray &partArray = *mParticles;

  // Create new TParticle and add it to the TParticle array
  Int_t trackId = mNumberOfEntriesInParticles;
  Int_t nPoints = 0;
  Int_t daughter1Id = -1;
  Int_t daughter2Id = -1;
  auto *particle = new(partArray[mNumberOfEntriesInParticles++])
    TParticle(pdgCode, trackId, parentId, nPoints, daughter1Id, daughter2Id, px, py, pz, e, vx, vy, vz, time);
  particle->SetPolarisation(polx, poly, polz);
  particle->SetWeight(weight);
  particle->SetUniqueID(proc);

  // Increment counter
  if (parentId < 0) {
    mNumberOfPrimaryParticles++;
  }

  // Set argument variable
  ntr = trackId;

  // Push particle on the stack if toBeDone is set
  if (toBeDone == 1) {
    mStack.push(particle);
  }
}

TParticle *Stack::PopNextTrack(Int_t &iTrack)
{

  // If end of stack: Return empty pointer
  if (mStack.empty()) {
    iTrack = -1;
    return nullptr;
  }

  // If not, get next particle from stack
  TParticle *thisParticle = mStack.top();
  mStack.pop();

  if (!thisParticle) {
    iTrack = 0;
    return nullptr;
  }

  mIndexOfCurrentTrack = thisParticle->GetStatusCode();
  iTrack = mIndexOfCurrentTrack;

  return thisParticle;
}

TParticle *Stack::PopPrimaryForTracking(Int_t iPrim)
{

  // Get the iPrimth particle from the mStack TClonesArray. This
  // should be a primary (if the index is correct).

  // Test for index
  if (iPrim < 0 || iPrim >= mNumberOfPrimaryParticles) {
    if (mLogger) {
      mLogger->Fatal(MESSAGE_ORIGIN, "Stack: Primary index out of range! %i ", iPrim);
    }
    Fatal("Stack::PopPrimaryForTracking", "Index out of range");
  }

  // Return the iPrim-th TParticle from the fParticle array. This should be
  // a primary.
  TParticle *part = (TParticle *) mParticles->At(iPrim);
  if (!(part->GetMother(0) < 0)) {
    if (mLogger) {
      mLogger->Fatal(MESSAGE_ORIGIN, "Stack:: Not a primary track! %i ", iPrim);
    }
    Fatal("Stack::PopPrimaryForTracking", "Not a primary track");
  }

  return part;
}

TParticle *Stack::GetCurrentTrack() const
{
  TParticle *currentPart = GetParticle(mIndexOfCurrentTrack);
  if (!currentPart) {
    if (mLogger) {
      mLogger->Warning(MESSAGE_ORIGIN, "Stack: Current track not found in stack!");
    }
    Warning("Stack::GetCurrentTrack", "Track not found in stack");
  }
  return currentPart;
}

void Stack::FillTrackArray()
{
  if (mLogger) {
    mLogger->Debug(MESSAGE_ORIGIN, "Stack: Filling MCTrack array...");
  } else {
    cout << "Stack: Filling MCTrack array..." << endl;
  }

  // Reset index map and number of output tracks
  mIndexMap.clear();
  mIndexVector.clear();
  mIndexVector.resize(mNumberOfEntriesInParticles, -2);
  mNumberOfEntriesInTracks = 0;

  // Check tracks for selection criteria
  SelectTracks();

  // Loop over mParticles array and copy selected tracks
  for (Int_t iPart = 0; iPart < mNumberOfEntriesInParticles; iPart++) {

    mStoreIterator = mStoreMap.find(iPart);
    if (mStoreIterator == mStoreMap.end()) {
      if (mLogger) {
        mLogger->Fatal(MESSAGE_ORIGIN, "Stack: Particle %i not found in storage map! ", iPart);
      }
      Fatal("Stack::FillTrackArray", "Particle not found in storage map.");
    }
    Bool_t store = (*mStoreIterator).second;

    if (store) {
      mTracks->emplace_back(GetParticle(iPart));
      auto& track = mTracks->back();			    
      mIndexMap[iPart] = mNumberOfEntriesInTracks;
      mIndexVector[iPart] = mNumberOfEntriesInTracks;

      // Set the number of points in the detectors for this track
      for (Int_t iDet = o2::Base::DetID::First; iDet < o2::Base::DetID::nDetectors; iDet++) {
        pair<Int_t, Int_t> a(iPart, iDet);
        track.setNumberOfPoints(iDet, mPointsMap[a]);
      }
      mNumberOfEntriesInTracks++;
    } else {
      mIndexMap[iPart] = -2;
    }
  }

  // Map index for primary mothers
  mIndexMap[-1] = -1;

  // Screen output
  // Print(1);
}

void Stack::UpdateTrackIndex(TRefArray *detList)
{
  // we are getting the detectorlist from FairRoot as TRefArray
  // at each call, but this list never changes so we cache it here
  // as the right type to avoid repeated dynamic casts
  if (mActiveDetectors.size() == 0) {
    if (detList == nullptr) {
      LOG(FATAL) << "No detList passed to Stack" << FairLogger::endl;
    }
    auto iter = detList->MakeIterator();
    while (auto det = iter->Next()) {
      auto o2det = dynamic_cast<o2::Base::Detector*>(det);
      if (o2det) {
        mActiveDetectors.emplace_back(o2det);
      } else {
        LOG(FATAL) << "Found nonconforming detector" << FairLogger::endl;
      }
    }
  }

  if (mLogger) {
    mLogger->Debug(MESSAGE_ORIGIN, "Stack: Updating track indices...");
  } else {
    cout << "Stack: Updating track indices..." << endl;
  }
  Int_t nColl = 0;

  // First update mother ID in MCTracks
  for (Int_t i = 0; i < mNumberOfEntriesInTracks; i++) {
    auto& track = (*mTracks)[i];
    Int_t iMotherOld = track.getMotherTrackId();
    mIndexIterator = mIndexMap.find(iMotherOld);
    if (mIndexIterator == mIndexMap.end()) {
      if (mLogger) {
        mLogger->Fatal(MESSAGE_ORIGIN, "Stack: Track index %i not found index map! ", iMotherOld);
      }
      Fatal("Stack::UpdateTrackIndex", "Track index not found in map");
    }
    track.SetMotherTrackId((*mIndexIterator).second);
  }

  for(auto det : mActiveDetectors) {
    // update the track indices by delegating to specialized detector functions
    det->updateHitTrackIndices(mIndexMap);
  } // List of active detectors

  if (mLogger) {
    mLogger->Debug(MESSAGE_ORIGIN, "...stack and  %i collections updated.", nColl);
  } else {
    cout << "...stack and  " << nColl << " collections updated." << endl;
  }
}

void Stack::Reset()
{
  mIndex = 0;
  mIndexOfCurrentTrack = -1;
  mNumberOfPrimaryParticles = mNumberOfEntriesInParticles = mNumberOfEntriesInTracks = 0;
  while (!mStack.empty()) {
    mStack.pop();
  }
  mParticles->Clear();
  mTracks->clear();
  mPointsMap.clear();
}

void Stack::Register()
{
  // LOG(INFO) << this << " register in "
  //   << FairGenericRootManager::Instance() << " mTracks: " <<  mTracks << std::endl;

  //  FairGenericRootManager::Instance()->Register("MCTrack", "Stack", mTracks, kTRUE);
  FairRootManager::Instance()->RegisterAny("MCTrack", mTracks, kTRUE);
}

void Stack::Print(Int_t iVerbose) const
{
  cout << "-I- Stack: Number of primaries        = " << mNumberOfPrimaryParticles << endl;
  cout << "              Total number of particles  = " << mNumberOfEntriesInParticles << endl;
  cout << "              Number of tracks in output = " << mNumberOfEntriesInTracks << endl;
  if (iVerbose) {
    for (auto& track : *mTracks) {
      track.Print();
    }
  }
}

void Stack::Print(Option_t* option) const
{
  Int_t verbose = 0;
  if ( option ) verbose = 1;
  Print(verbose);
}

void Stack::AddPoint(int iDet)
{
  // cout << "Add point for Detektor" << iDet << endl;
  pair<Int_t, Int_t> a(mIndexOfCurrentTrack, iDet);
  if (mPointsMap.find(a) == mPointsMap.end()) {
    mPointsMap[a] = 1;
  } else {
    mPointsMap[a]++;
  }
}

void Stack::AddPoint(int iDet, Int_t iTrack)
{
  if (iTrack < 0) {
    return;
  }
  pair<Int_t, Int_t> a(iTrack, iDet);
  if (mPointsMap.find(a) == mPointsMap.end()) {
    mPointsMap[a] = 1;
  } else {
    mPointsMap[a]++;
  }
}

Int_t Stack::GetCurrentParentTrackNumber() const
{
  TParticle *currentPart = GetCurrentTrack();
  if (currentPart) {
    return currentPart->GetFirstMother();
  } else {
    return -1;
  }
}

TParticle *Stack::GetParticle(Int_t trackID) const
{
  if (trackID < 0 || trackID >= mNumberOfEntriesInParticles) {
    if (mLogger) {
      mLogger->Debug(MESSAGE_ORIGIN, "Stack: Particle index %i out of range.", trackID);
    }
    Fatal("Stack::GetParticle", "Index out of range");
  }
  return (TParticle *) mParticles->At(trackID);
}

void Stack::SelectTracks()
{

  // Clear storage map
  mStoreMap.clear();

  // LOG(INFO) << "mPointsMap.size(): " << mPointsMap.size() << std::endl;

  // Check particles in the fParticle array
  for (Int_t i = 0; i < mNumberOfEntriesInParticles; i++) {

    TParticle *thisPart = GetParticle(i);
    Bool_t store = kTRUE;

    // Get track parameters
    Int_t iMother = thisPart->GetMother(0);
    TLorentzVector p;
    thisPart->Momentum(p);
    Double_t energy = p.E();
    Double_t mass = p.M();
    //    Double_t mass   = thisPart->GetMass();
    Double_t eKin = energy - mass;

    // Calculate number of points
    Int_t nPoints = 0;
    for (Int_t iDet = o2::Base::DetID::First; iDet < o2::Base::DetID::nDetectors; iDet++) {
      pair<Int_t, Int_t> a(i, iDet);
      if (mPointsMap.find(a) != mPointsMap.end()) {
        nPoints += mPointsMap[a];
      }
    }

    // Check for cuts (store primaries in any case)
    if (iMother < 0) {
      store = kTRUE;
    } else {
      if (!mStoreSecondaries) {
        store = kFALSE;
      }
      if (nPoints < mMinPoints) {
        store = kFALSE;
      }
      if (eKin < mEnergyCut) {
        store = kFALSE;
      }
    }

    // Set storage flag
    mStoreMap[i] = store;
  }

  // If flag is set, flag recursively mothers of selected tracks
  if (mStoreMothers) {
    for (Int_t i = 0; i < mNumberOfEntriesInParticles; i++) {
      if (mStoreMap[i]) {
        Int_t iMother = GetParticle(i)->GetMother(0);
        while (iMother >= 0) {
          mStoreMap[iMother] = kTRUE;
          iMother = GetParticle(iMother)->GetMother(0);
        }
      }
    }
  }
}

FairGenericStack *Stack::CloneStack() const
{
  return new o2::Data::Stack(*this);
}

ClassImp(o2::Data::Stack)
