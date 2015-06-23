// -- author: Xin Shi <Xin.Shi@cern.ch>  
// tests run on Purdue University 
#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixUtil.hh"
#include "PixTestFactory.hh"
#include "PixTestPurdueTest.hh"
#include "log.h"

#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestPurdueTest)

// ----------------------------------------------------------------------
PixTestPurdueTest::PixTestPurdueTest(PixSetup *a, std::string name) : PixTest(a, name) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestPurdueTest ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestPurdueTest::PixTestPurdueTest() : PixTest() {
  LOG(logDEBUG) << "PixTestPurdueTest ctor()";
}

// ----------------------------------------------------------------------
bool PixTestPurdueTest::setParameter(string parName, string /*sval*/) {
  bool found(false);
  string stripParName; 
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      if (!parName.compare("deadface")) {
	//	fDeadFace = static_cast<uint16_t>(atoi(sval.c_str())); 
	setToolTips();
      }
      break;
    }
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTestPurdueTest::init() {
  LOG(logDEBUG) << "PixTestPurdueTest::init()";

  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}

// ----------------------------------------------------------------------
void PixTestPurdueTest::setToolTips() {
  fTestTip    = string("run the complete PurdueTest")
    ;
  fSummaryTip = string("to be implemented")
    ;
}


// ----------------------------------------------------------------------
void PixTestPurdueTest::bookHist(string name) {
  LOG(logDEBUG) << "nothing done with " << name;
  fDirectory->cd(); 
}


//----------------------------------------------------------
PixTestPurdueTest::~PixTestPurdueTest() {
  LOG(logDEBUG) << "PixTestPurdueTest dtor";
}


// ----------------------------------------------------------------------
void PixTestPurdueTest::doTest() {

  bigBanner(Form("PixTestPurdueTest::doTest()"));

  vector<string> suite;
  suite.push_back("pretest"); 
  suite.push_back("alive"); 
  suite.push_back("trim"); 
  suite.push_back("bb"); 

  // suite.push_back("scurves");
  // suite.push_back("phoptimization"); 
  // suite.push_back("gainpedestal"); 

  PixTest *t(0); 

  string trimvcal(""); 
  PixTestFactory *factory = PixTestFactory::instance(); 
  for (unsigned int i = 0; i < suite.size(); ++i) {
    t =  factory->createTest(suite[i], fPixSetup);

    if (!suite[i].compare("trim")) {
      trimvcal = t->getParameter("vcal"); 
      fPixSetup->getConfigParameters()->setTrimVcalSuffix(trimvcal, true); 
    }

    t->fullTest(); 

    delete t; 
  }

}


