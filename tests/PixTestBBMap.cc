// -- author: Wolfram Erdmann
// Bump Bonding tests, really just a threshold map using cals

#include <sstream>   // parsing
#include <algorithm>  // std::find

#include <TArrow.h>
#include <TSpectrum.h>
#include "TStopwatch.h"
#include "TStyle.h"

#include "PixTestBBMap.hh"
#include "PixUtil.hh"
#include "log.h"
#include "constants.h"   // roctypes

using namespace std;
using namespace pxar;

ClassImp(PixTestBBMap)

//------------------------------------------------------------------------------
PixTestBBMap::PixTestBBMap(PixSetup *a, std::string name): PixTest(a, name), 
  fParNtrig(0), fParVcalS(200), fDumpAll(-1), fDumpProblematic(-1) {
  PixTest::init();
  init();
  LOG(logDEBUG) << "PixTestBBMap ctor(PixSetup &a, string, TGTab *)";
}


//------------------------------------------------------------------------------
PixTestBBMap::PixTestBBMap(): PixTest() {
  LOG(logDEBUG) << "PixTestBBMap ctor()";
}



//------------------------------------------------------------------------------
bool PixTestBBMap::setParameter(string parName, string sval) {

  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (uint32_t i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());

      stringstream s(sval);
      if (!parName.compare( "ntrig")) { 
	s >> fParNtrig; 
	setToolTips();
	return true;
      }
      if (!parName.compare( "vcals")) { 
	s >> fParVcalS; 
	setToolTips();
	return true;
      }

      if (!parName.compare("dumpall")) {
	PixUtil::replaceAll(sval, "checkbox(", ""); 
	PixUtil::replaceAll(sval, ")", ""); 
	fDumpAll = atoi(sval.c_str()); 
	setToolTips();
      }

      if (!parName.compare("dumpallproblematic")) {
	PixUtil::replaceAll(sval, "checkbox(", ""); 
	PixUtil::replaceAll(sval, ")", ""); 
	fDumpProblematic = atoi(sval.c_str()); 
	setToolTips();
      }

    }
  }
  return false;
}

//------------------------------------------------------------------------------
void PixTestBBMap::init() {
  LOG(logDEBUG) << "PixTestBBMap::init()";
  
  fDirectory = gFile->GetDirectory("BumpBonding");
  if (!fDirectory) {
    fDirectory = gFile->mkdir("BumpBonding");
  }
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestBBMap::setToolTips() {
  fTestTip = string( "Bump Bonding Test = threshold map for CalS");
  fSummaryTip = string("module summary");
}


//------------------------------------------------------------------------------
PixTestBBMap::~PixTestBBMap() {
  LOG(logDEBUG) << "PixTestBBMap dtor";
}

//------------------------------------------------------------------------------
void PixTestBBMap::doTest() {

  TStopwatch t;

  gStyle->SetPalette(1);
  cacheDacs();
  PixTest::update();
  bigBanner(Form("PixTestBBMap::doTest() Ntrig = %d, VcalS = %d (high range)", fParNtrig, fParVcalS));
 
  fDirectory->cd();

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();

  int flag(FLAG_CALS);
  fApi->setDAC("ctrlreg", 4);     // high range
  fApi->setDAC("vcal", fParVcalS);    

  int result(1);
  if (fDumpAll) result |= 0x20;
  if (fDumpProblematic) result |= 0x10;

  fNDaqErrors = 0; 
  vector<TH1*>  thrmapsCals = scurveMaps("VthrComp", "calSMap", fParNtrig, 0, 149, -1, -1, result, 1, flag);

  // -- relabel negative thresholds as 255 and create distribution list
  vector<TH1D*> dlist; 
  TH1D *h(0);
  for (unsigned int i = 0; i < thrmapsCals.size(); ++i) {
    for (int ix = 0; ix < thrmapsCals[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < thrmapsCals[i]->GetNbinsY(); ++iy) {
	if (thrmapsCals[i]->GetBinContent(ix+1, iy+1) < 0) thrmapsCals[i]->SetBinContent(ix+1, iy+1, 255.);
      }
    }
    h = distribution((TH2D*)thrmapsCals[i], 256, 0., 256.);

    dlist.push_back(h); 
    fHistList.push_back(h); 
  }

  restoreDacs();

  // -- summary printout
  string bbString(""), bbCuts(""); 
  int bbprob(0); 
  int nPeaks(0), cutDead(0); 
  TSpectrum s; 
  for (unsigned int i = 0; i < dlist.size(); ++i) {
    h = (TH1D*)dlist[i];
    nPeaks = s.Search(h, 5, "", 0.01); 
    LOG(logDEBUG) << "found " << nPeaks << " peaks in " << h->GetName();
    cutDead = fitPeaks(h, s, nPeaks); 

    bbprob = static_cast<int>(h->Integral(cutDead, h->FindBin(255)));
    bbString += Form(" %4d", bbprob); 
    bbCuts   += Form(" %4d", cutDead); 

    TArrow *pa = new TArrow(cutDead, 0.5*h->GetMaximum(), cutDead, 0., 0.06, "|>"); 
    pa->SetArrowSize(0.1);
    pa->SetAngle(40);
    pa->SetLineWidth(2);
    h->GetListOfFunctions()->Add(pa); 

  }

  if (h) {
    h->Draw();
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  }
  PixTest::update(); 
  
  int seconds = t.RealTime();
  LOG(logINFO) << "PixTestBBMap::doTest() done"
	       << (fNDaqErrors>0? Form(" with %d decoding errors: ", static_cast<int>(fNDaqErrors)):"") 
	       << ", duration: " << seconds << " seconds";
  LOG(logINFO) << "number of dead bumps (per ROC): " << bbString;
  LOG(logINFO) << "separation cut       (per ROC): " << bbCuts;
  dutCalibrateOff();

}



// ----------------------------------------------------------------------
int PixTestBBMap::fitPeaks(TH1D *h, TSpectrum &s, int npeaks) {

#if defined ROOT_MAJOR_VER && ROOT_MAJOR_VER > 5
  Double_t *xpeaks = s.GetPositionX();
#else
  Float_t *xpeaks = s.GetPositionX();
#endif

  string name; 
  double lcuts[3]; lcuts[0] = lcuts[1] = lcuts[2] = 255.;
  TF1 *f(0); 
  double peak, sigma;
  int fittedPeaks(0); 
  for (int p = 0; p < npeaks; ++p) {
    double xp = xpeaks[p];
    if (p > 1) continue;
    if (xp > 200) {
      continue;
    }
    if (xp < 15) {
      continue;
    }
    name = Form("gauss_%d", p); 
    f = new TF1(name.c_str(), "gaus(0)", 0., 256.);
    int bin = h->GetXaxis()->FindBin(xp);
    double yp = h->GetBinContent(bin);
    f->SetParameters(yp, xp, 2.);
    h->Fit(f, "Q+"); 
    ++fittedPeaks;
    peak = h->GetFunction(name.c_str())->GetParameter(1); 
    sigma = h->GetFunction(name.c_str())->GetParameter(2); 
    if (0 == p) {
      lcuts[0] = peak + 3*sigma; 
      if (h->Integral(h->FindBin(peak + 10.*sigma), 250) > 10.) {
	lcuts[1] = peak + 5*sigma;
      } else {
	lcuts[1] = peak + 10*sigma;
      }
    } else {
      lcuts[1] = peak - 3*sigma; 
      lcuts[2] = peak - sigma; 
    }
    delete f;
  }
  
  int startbin = (int)(0.5*(lcuts[0] + lcuts[1])); 
  int endbin = (int)(lcuts[1]); 
  if (endbin <= startbin) {
    endbin = (int)(lcuts[2]); 
    if (endbin < startbin) {
      endbin = 255.;
    }
  }

  int minbin(0); 
  double minval(999.); 
  
  for (int i = startbin; i <= endbin; ++i) {
    if (h->GetBinContent(i) < minval) {
      if (1 == fittedPeaks) {
	if (0 == h->Integral(i, i+4)) {
	  minval = h->GetBinContent(i); 
	  minbin = i; 
	} else {
	  minbin = endbin;
	}
      } else {
	minval = h->GetBinContent(i); 
	minbin = i; 
      }
    }
  }
  
  LOG(logDEBUG) << "cut for dead bump bonds: " << minbin << " (obtained for minval = " << minval << ")" 
		<< " start: " << startbin << " .. " << endbin 
		<< " last peak: " << peak << " last sigma: " << sigma
		<< " lcuts[0] = " << lcuts[0] << " lcuts[1] = " << lcuts[1];
  return minbin+1; 
}
