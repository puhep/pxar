// to send different patterns and check the readout

#include <stdlib.h>   // atof, atoi
#include <algorithm>  // std::find
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "PixTestPattern.hh"

#include "log.h"
#include "constants.h"
#include "helper.h"
#include "timer.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestPattern)

//------------------------------------------------------------------------------
PixTestPattern::PixTestPattern(PixSetup *a, std::string name) : PixTest(a, name), fParPgCycles(0), fParTrigLoop(0), fParPeriod(0), fParSeconds(0), fPatternFromFile(0), fResultsOnFile(1), fBinOut(0), fFileName("null"), fUnMaskAll(0), fParFillTree(0){
	PixTest::init();
	init();
	LOG(logDEBUG) << "PixTestPattern ctor(PixSetup &a, string, TGTab *)";
}

//------------------------------------------------------------------------------
PixTestPattern::PixTestPattern() : PixTest(){ //ctor
}

//------------------------------------------------------------------------------
bool PixTestPattern::setParameter(string parName, string sval)
{
	bool found(false);
	fParOutOfRange = false;
	std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
	for (unsigned int i = 0; i < fParameters.size(); ++i)
	{
		if (fParameters[i].first == parName)
		{
			found = true;
			sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());

			if (!parName.compare("pgcycles")){
				fParPgCycles = atoi(sval.c_str());
				setToolTips();
				LOG(logDEBUG) << "  setting pgcycles -> " << fParPgCycles;
				if (fParPgCycles < 0) {
					LOG(logWARNING) << "PixTestPattern::setParameter() pg_cycles must be positive";
					found = false; fParOutOfRange = true;
				}
			}

			if (!parName.compare("triggerloop")) {
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fParTrigLoop = atoi(sval.c_str());
				setToolTips();
				LOG(logDEBUG) << "  setting fParTrigLoop -> " << fParTrigLoop;
			}

			if (!parName.compare("period")){
				fParPeriod = atoi(sval.c_str());
				setToolTips();
				LOG(logDEBUG) << "  setting fParPeriod -> " << fParPeriod;
				if (fParPeriod < 0) {
					LOG(logWARNING) << "PixTestPattern::setParameter() period must be positive";
					found = false; fParOutOfRange = true;
				}
			}

			if (!parName.compare("seconds")){
				fParSeconds = atoi(sval.c_str());
				setToolTips();
				LOG(logDEBUG) << "  setting fParSeconds -> " << fParSeconds;
				if (fParSeconds < 0) {
					LOG(logWARNING) << "PixTestPattern::setParameter() seconds must be positive";
					found = false; fParOutOfRange = true;
				}
			}

			if (!parName.compare("patternfromfile")) {
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fPatternFromFile = atoi(sval.c_str());
				setToolTips();
				LOG(logDEBUG) << "  setting fPatternFromFile -> " << fPatternFromFile;
			}

			if (!parName.compare("resultsonfile")){
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fResultsOnFile = atoi(sval.c_str());
				setToolTips();
				LOG(logDEBUG) << "  setting fResultsOnFile -> " << fResultsOnFile;
			}

			if (!parName.compare("binaryoutput")){
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fBinOut = atoi(sval.c_str());
				setToolTips();
				LOG(logDEBUG) << "  setting fBinOut -> " << fBinOut;
			}

			if (!parName.compare("filename")){
				fFileName = sval.c_str();
				setToolTips();
				LOG(logDEBUG) << "  setting fFileName -> " << fFileName;
			}

			if (!parName.compare("unmaskall")){
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fUnMaskAll = atoi(sval.c_str());
				setToolTips();
				LOG(logDEBUG) << "  setting fUnMaskAll -> " << fUnMaskAll;
			}

			if (!parName.compare("filltree")) {
				fParFillTree = !(atoi(sval.c_str()) == 0);
				setToolTips();
				LOG(logINFO) << "  setting fParFillTree -> " << fParFillTree;
			}

			break;
		}
	}
	return found;
}

//------------------------------------------------------------------------------
void PixTestPattern::init()
{
	LOG(logDEBUG) << "PixTestPattern::init()";
	setToolTips();
	fDirectory = gFile->GetDirectory(fName.c_str());
	if (!fDirectory)
		fDirectory = gFile->mkdir(fName.c_str());
	fDirectory->cd();
	fTree = 0;
}

// ----------------------------------------------------------------------
void PixTestPattern::setToolTips() {

	fTestTip = string("run DAQ");
	fSummaryTip = string("Show summary plot");
	fStopTip = string("Stop DAQ");
}

//------------------------------------------------------------------------------
PixTestPattern::~PixTestPattern(){ //dctor
	fDirectory->cd();
	if (fTree && fParFillTree) fTree->Write();
}

// ----------------------------------------------------------------------
void PixTestPattern::stop()
{
	// Interrupt the test 
	fDaq_loop = false;
	LOG(logINFO) << "PixTestPattern:: STOP PRESSED. Ending test.";
}

// ----------------------------------------------------------------------
void PixTestPattern::runCommand(std::string command) {
	std::transform(command.begin(), command.end(), command.begin(), ::tolower);
	LOG(logDEBUG) << "running command: " << command;

	if (!command.compare("resettodefault")) {
		LOG(logINFO) << "PixTestPattern:: reset parameters from testParameters.dat";
		for (unsigned int i = 0; i < fParameters.size(); ++i)
			setParameter(fParameters[i].first, fParameters[i].second);
		return;
	}

	else if (!command.compare("stop")){
		stop();
	}

	else	LOG(logDEBUG) << "did not find command ->" << command << "<-";
}

// ----------------------------------------------------------------------
bool PixTestPattern::setPattern(string fname) {

	ifstream is(fname.c_str());
	if (!is.is_open()) {
		LOG(logWARNING) << "PixTestPattern::setPattern() cannot read " << fname;
		return false;
	}

	bool patternFound(false);
	string line;

	while (is.good())
	{
		getline(is, line);

		// -- find Pattern section
		if (string::npos != line.find("-- Pattern")){
			patternFound = true;
			continue;
		}

		if (string::npos != line.find("-- Test Pixels")) break;

		if (patternFound)
		{
			std::string::size_type sep;
			std::string sig, str;
			int val(0);

			// -- remove tabs, adjacent spaces, leading and trailing spaces
			PixUtil::replaceAll(line, "\t", " ");
			string::iterator new_end = unique(line.begin(), line.end(), PixUtil::bothAreSpaces);
			line.erase(new_end, line.end());
			if (line.length() < 2) continue;

			sep = line.find(",");

			if (string::npos != sep)
			{
				sig = line.substr(0, sep - 1);
				str = line.substr(sep + 1);
				val = atoi(str.c_str());

				//check if delay stays within 8bit
				if (val < 0 || val > 255) {
					LOG(logWARNING) << "PixTestPattern::setPattern() delay out of range [0,255]";
					return false;
				}
				uint8_t del = val;
				fPg_setup.push_back(make_pair(sig, del));
				LOG(logDEBUG) << "  pg set to -> \"" << sig << "\" " << del;
			}

			else
			{
				fPg_setup.push_back(make_pair("", 0));
				LOG(logWARNING) << "PixTestPattern::setPattern() wrong ... "; //DEBUG
			}
		}

	}

	if (!patternFound){
		LOG(logWARNING) << "PixTestPattern::setPattern()  '-- Pattern' not found in testPattern.dat";
		fPg_setup.push_back(make_pair("", 0));
		return false;
	}

	return true;
}

// ----------------------------------------------------------------------
bool PixTestPattern::setPixels(string fname, string flag) {

	int npix = 0;
	std::stringstream sstr;

	ifstream is(fname.c_str());
	if (!is.is_open()) {
		LOG(logWARNING) << "PixTestPattern::setPixels() cannot read " << fname;
		return false;
	}

	bool pixelFound(false);
	string line;

	while (is.good())
	{
		getline(is, line);

		if (flag == "Test")
		{
			// -- find Test Pixels section
			if (string::npos != line.find("-- Test Pixels")) {
				pixelFound = true;
				continue;
			}

			if (string::npos != line.find("-- Unmask Pixels")) break;
		}
		else
		{
			// -- find Unmask Pixels section
			if (string::npos != line.find("-- Unmask Pixels")) {
				pixelFound = true;
				continue;
			}
		}

		if (pixelFound)
		{
			int pixc, pixr;
			std::string::size_type s0, s1;
			std::string str1, str2;

			// -- remove tabs, adjacent spaces, leading and trailing spaces
			PixUtil::replaceAll(line, "\t", " ");
			string::iterator new_end = unique(line.begin(), line.end(), PixUtil::bothAreSpaces);
			line.erase(new_end, line.end());
			if (line.length() < 2) continue;

			s0 = line.find(" ");
			s1 = line.find(",");
			if (string::npos != s1)
			{
				str1 = line.substr(s0, s1);
				pixc = atoi(str1.c_str());
				str2 = line.substr(s1 + 1);
				pixr = atoi(str2.c_str());
				if (flag == "Test")	fPIX.push_back(make_pair(pixc, pixr));
				else				fPIXm.push_back(make_pair(pixc, pixr));
				sstr << " (" << pixc << "," << pixr << ")";
				npix++;
			}
			else
			{
				fPIX.push_back(make_pair(-1, -1));
				fPIXm.push_back(make_pair(-1, -1));
				cout << "(null)";
			}
		}
	}

	if (!pixelFound)
	{
		if (flag == "Test")	{
			LOG(logWARNING) << "PixTestPattern::setPixels()  '-- Test Pixels' not found in testPattern.dat";
			fPIX.push_back(make_pair(-1, -1));
		}
		else {
			LOG(logWARNING) << "PixTestPattern::setPixels()  '-- Unmask Pixels' not found in testPattern.dat";
			fPIXm.push_back(make_pair(-1, -1));
		}
		return false;
	}
	
	if (flag == "Test")	LOG(logINFO) << "PixTestPattern:: " << npix << " 'armed'  pixels:" << sstr.str();
	else				LOG(logINFO) << "PixTestPattern:: " << npix << " unmasked pixels:" << sstr.str();
	sstr.clear();

	return true;
}

// ----------------------------------------------------------------------
void PixTestPattern::setHistos(){

	if (fParFillTree) bookTree();
	std::vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();

	for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
		fH2 = bookTH2D(Form("hits_C%d", rocIds[iroc]), Form("hits_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
		fH2->SetMinimum(0.);
		fH2->SetDirectory(fDirectory);
		setTitles(fH2, "col", "row");
		fHistOptions.insert(make_pair(fH2, "colz"));
		fHits.push_back(fH2);

		fP2 = bookTProfile2D(Form("phMap_C%d", rocIds[iroc]), Form("phMap_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
		fP2 ->SetMinimum(0.);
		fP2->SetDirectory(fDirectory);
		setTitles(fP2, "col", "row");
		fHistOptions.insert(make_pair(fP2, "colz"));
		fPhmap.push_back(fP2);

		fH1 = bookTH1D(Form("ph_C%d", rocIds[iroc]), Form("ph_C%d", rocIds[iroc]), 256, 0., 256.);
		fH1->SetMinimum(0.);
		fH1->SetDirectory(fDirectory);
		setTitles(fH1, "ADC", "Entries/bin");
		fPh.push_back(fH1);
	}
}

// ----------------------------------------------------------------------
void PixTestPattern::FillHistos(std::vector<pxar::Event> data) {	
		std::vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
		int pixCnt(0);
		int idx(-1);
		std::vector<uint8_t> cnt;
		//not to fill always the first histo:
		for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc)
			cnt.push_back((uint8_t)(histCycle(Form("hits_C%d", iroc)) - 1));
		
		for (std::vector<pxar::Event>::iterator it = data.begin(); it != data.end(); ++it) {

			for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){

				if (fParFillTree) {
					fTreeEvent.header = it->header;
					fTreeEvent.dac = 0;
					fTreeEvent.trailer = it->trailer;
					fTreeEvent.numDecoderErrors = it->numDecoderErrors;
					fTreeEvent.npix = it->pixels.size();
				}
				for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {
					idx = getIdxFromId(it->pixels[ipix].roc_id) + cnt[iroc];
					fHits[idx]->Fill(it->pixels[ipix].column, it->pixels[ipix].row);
					fPhmap[idx]->Fill(it->pixels[ipix].column, it->pixels[ipix].row, it->pixels[ipix].value);
					fPh[idx]->Fill(it->pixels[ipix].value);
					if (fParFillTree) {
						fTreeEvent.proc[ipix] = it->pixels[ipix].roc_id;
						fTreeEvent.pcol[ipix] = it->pixels[ipix].column;
						fTreeEvent.prow[ipix] = it->pixels[ipix].row;
						fTreeEvent.pval[ipix] = it->pixels[ipix].value;
						fTreeEvent.pq[ipix] = 0; //no charge..
					}
				}				
				if (fParFillTree) fTree->Fill();
			}
		}

		//to draw the hitsmap as 'online' check.
		fH2 = (TH2D*)(fHits.back());
		fH2->Draw(getHistOption(fH2).c_str());
		PixTest::update();
}

// ----------------------------------------------------------------------
void PixTestPattern::PrintEvents(int par1, int par2, string flag) {

	std::vector<pxar::Event> daqEvBuffer;
	size_t daqEvBuffsiz;

	if (!fResultsOnFile)
	{
		daqEvBuffer = fApi->daqGetEventBuffer();
		daqEvBuffsiz = daqEvBuffer.size();

		FillHistos(daqEvBuffer); //fill&print histos on the gui

		if (daqEvBuffsiz <= 201) {
			LOG(logINFO) <<  "PixTestPattern:: data from buffer:";
			cout << endl;
			for (unsigned int i = 0; i < daqEvBuffsiz; i++)	{
				cout << i << " : " << daqEvBuffer[i] << endl;
			}
			cout << endl;
		}
		else {
			cout << endl;
			LOG(logINFO) << "PixTestPattern:: data from buffer:";
			for (unsigned int i = 0; i <= 100; i++)	{
				cout << i << " : " << daqEvBuffer[i] << endl;
			}
			//skip events. If you want all the events printed select 'binaryoutput'.
			cout << endl << "................... SKIP EVENTS TO NOT SATURATE THE SHELL ...................." << endl << endl;
			for (unsigned int i = (daqEvBuffsiz - 100); i < daqEvBuffsiz; i++)	{
				cout << i << " : " << daqEvBuffer[i] << endl;
			}
			cout << endl;
		}
		LOG(logINFO) << "PixTestPattern:: " << daqEvBuffsiz << " events read from buffer";
		cout << endl;
	}

	else
	{
		LOG(logINFO) << "PixTestPattern:: Start reading data from DTB RAM";
		std::stringstream sstr, sdata;
		string FileName;
		if (flag == "trg") sstr << "_" << par1 << "pgCycles";
		else sstr << "_" << par1 << "sec" << "_" << par2;
		if (fBinOut) FileName = f_Directory + "/" + fFileName.c_str() + sstr.str() + ".bin";
		else FileName = f_Directory + "/" + fFileName.c_str() + sstr.str() + ".dat";


		if (fBinOut)
		{
			std::vector<uint16_t> daqdat = fApi->daqGetBuffer();
			if (daqdat.size() > 550000) sdata << (daqdat.size() / 524288) << "MB";
			else sdata << (daqdat.size() / 512) << "kB";
			LOG(logINFO) << "PixTestPattern:: " << daqdat.size() << " words of data read : " << sdata.str();
			std::ofstream fout(FileName.c_str(), std::ios::out | std::ios::binary);
			LOG(logINFO) << "PixTestPattern:: Writing binary";
			fout.write(reinterpret_cast<const char*>(&daqdat[0]), sizeof(daqdat[0])*daqdat.size());
			fout.close();
		}

		else
		{
			daqEvBuffer = fApi->daqGetEventBuffer();
			daqEvBuffsiz = daqEvBuffer.size();
			LOG(logINFO) << "PixTestPattern:: " << daqEvBuffsiz << " events read";

			FillHistos(daqEvBuffer); //fill&print histos on the gui

			std::ofstream fout(FileName.c_str(), std::ofstream::out);
			if (daqEvBuffsiz <= 201) {
				LOG(logINFO) << "PixTestPattern:: Writing decoded events";
				for (unsigned int i = 0; i < daqEvBuffsiz; ++i)	{
					fout << i << " : " << daqEvBuffer[i] << endl;
				}
			}
			else {
				LOG(logINFO) << "PixTestPattern:: Writing decoded events (a fraction of)";
				for (unsigned int i = 0; i <= 100; i++)	{
					fout << i << " : " << daqEvBuffer[i] << endl;
				}
				//skip events. If you want all the events printed select 'binaryoutput'.
				fout << endl << "................... SKIP EVENTS TO NOT TAKE TOO LONG ...................." << endl << endl;
				for (unsigned int i = (daqEvBuffsiz - 100); i < daqEvBuffsiz; i++)	{
					fout << i << " : " << daqEvBuffer[i] << endl;
				}
			}
			fout.close();
		}

		LOG(logINFO) << "PixTestPattern:: Wrote data to " << FileName.c_str();
		cout << endl;
		FileName.clear();
	}

}

// ----------------------------------------------------------------------
void PixTestPattern::TriggerLoop(int checkfreq) {

	uint8_t perFull;
	int nloop = 1;
	uint64_t diff = 0, timepaused = 0, timeff = 0;
	bool TotalTime = false;
	timer t;
	LOG(logINFO) << "PixTestPattern:: starting TriggerLoop for " << fParSeconds << " seconds";

	while (fDaq_loop)
	{
		if (nloop > 1){
			diff = t.get() - diff;
			timepaused += diff;
			LOG(logDEBUG) << "PixTestPattern:: readout time " << timepaused / 1000 << " seconds";
			LOG(logINFO) << "PixTestPattern:: restarting TriggerLoop for " << fParSeconds - (timeff / 1000) << " s";
		}
		//start triggerloop:
		fPeriod = fApi->daqTriggerLoop(fParPeriod);
		if (nloop == 1) LOG(logINFO) << "PixTestPattern:: TriggerLoop period = " << fPeriod << " clks";
		
		//check every checkfreq seconds if buffer is full less then 80%:
		while (fApi->daqStatus(perFull) && perFull < 80 && fDaq_loop) {
			mDelay(checkfreq * 1000);
			timeff = t.get() - timepaused;
			LOG(logINFO) << "PixTestPattern:: elapsed time " << timeff / 1000 << " seconds";
			if (timeff / 1000 >= fParSeconds)       {
				fDaq_loop = false;
				TotalTime = true;
				break;
			}
			LOG(logINFO) << "PixTestPattern:: buffer not full, at " << (int)perFull << "%";
			gSystem->ProcessEvents();
		}

		if (fDaq_loop) {
			LOG(logINFO) << "PixTestPattern:: buffer almost full, pausing triggers";
			fApi->daqTriggerLoopHalt();
			diff = t.get();
		}
		else {
				if (TotalTime) { LOG(logINFO) << "PixTestPattern:: total time reached - DAQ stopped."; }
				//fApi->daqTriggerLoopHalt();
				fApi->daqStop();			
		}
		// Get events and Print results on shell/file:
		PrintEvents(fParSeconds, nloop, "loop");
		nloop++;
	}
}

// ----------------------------------------------------------------------
void PixTestPattern::pgToDefault() {
	fPg_setup.clear();
	fPg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();
	fApi->setPatternGenerator(fPg_setup);
	LOG(logINFO) << "PixTestPattern:: pg_setup set to default";
}

// ----------------------------------------------------------------------
void PixTestPattern::FinalCleaning() {
	// Reset the pg_setup to default value.
	pgToDefault();

	//clean local variables:
	fPIX.clear();
	fPIXm.clear();
	fPg_setup.clear();		
}

//------------------------------------------------------------------------------
void PixTestPattern::doTest()
{
	fDirectory->cd();
	fHistList.clear(); //needed
	fPg_setup.clear();
	PixTest::update();

	//setparameters and check if in range	
	if (fParOutOfRange) return;

	LOG(logINFO) << "PixTestPattern::doTest() start";	

	//set the input filename (for Pattern and Pixels)
	string fname;
	ConfigParameters* config = ConfigParameters::Singleton();
	f_Directory = config->getDirectory();
	fname = f_Directory + "/testPatterns.dat";

	// to unmask all or only selected pixels:
	if (fUnMaskAll) {
		fApi->_dut->maskAllPixels(false);
		LOG(logINFO) << "PixTestPattern:: all pixels unmasked";
	}
	else {
		
		fApi->_dut->maskAllPixels(true);
		if (!setPixels(fname, "Unmask")){   //READ FROM FILE	
			FinalCleaning();
			return;
		}
		for (unsigned int i = 0; i < fPIXm.size(); ++i)	{
			if (fPIXm[i].first > -1)  fApi->_dut->maskPixel(fPIXm[i].first, fPIXm[i].second, false);
		}
	}
	
	// to 'arm' only selected pixels:
	fPIX.clear();
	if (!setPixels(fname, "Test")){   //READ FROM FILE	
		FinalCleaning();
		return;    
	}
	fApi->_dut->testAllPixels(false);
	for (unsigned int i = 0; i < fPIX.size(); ++i) {
	 	if (fPIX[i].first > -1)	{
				fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, true);
				fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, false);
			}
			else {
				fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, true);
			}
		}
	
	//set the histos
	setHistos();

	// Start the DAQ:
	//::::::::::::::::::::::::::::::::

	//first send only a RES:
	fPg_setup.push_back(make_pair("resetroc", 0));
	fPeriod = 28;

	// Set the pattern generator:
	fApi->setPatternGenerator(fPg_setup);

	fApi->daqStart();

	//send only one trigger to reset:
	fApi->daqTrigger(1, fPeriod);
	LOG(logINFO) << "PixTestPattern:: RES sent once ";

	fPg_setup.clear();
	LOG(logINFO) << "PixTestPattern:: pg_setup clean";

	//select the pattern:
	if (fPatternFromFile)
	{
		LOG(logINFO) << "PixTestPattern:: Set pattern from file: " << fname;
		if (!setPattern(fname)){   //READ FROM FILE	
			fApi->daqStop();
			FinalCleaning();
			return;
		}
	}
	else {    //standard pattern from config parameters.
		fPg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();
	}

	//set pattern generator:
	fApi->setPatternGenerator(fPg_setup);
	fPeriod = 0;

	//send Triggers (loop or single) wrt parameters selection:
	if (!fParTrigLoop) {
		//pg_cycles times the pg_Single() == pg_cycles times pattern sequence):
		fPeriod = fApi->daqTrigger(fParPgCycles, fParPeriod);
		LOG(logINFO) << "PixTestPattern:: " << fParPgCycles << " pg_Single() sent with period " << fPeriod;

		fApi->daqStop();

		// Get events and Print results on shell/file:
		PrintEvents(fParPgCycles, 0, "trg");
	}
	else {
		fDaq_loop = true;
		TriggerLoop(2); //argument == buffer check frequency (seconds)
	}

	//::::::::::::::::::::::::::::::
	//DAQ - THE END.

	copy(fPh.begin(), fPh.end(), back_inserter(fHistList));
	fH1 = (TH1D*)(fHistList.back());
	fH1->Draw(getHistOption(fH1).c_str());
	fDisplayedHist = find(fHistList.begin(), fHistList.end(), fH1);

	copy(fPhmap.begin(), fPhmap.end(), back_inserter(fHistList));
	fP2 = (TProfile2D*)(fHistList.back());
	fP2->Draw(getHistOption(fP2).c_str());
	fDisplayedHist = find(fHistList.begin(), fHistList.end(), fP2);

	copy(fHits.begin(), fHits.end(), back_inserter(fHistList));
	fH2 = (TH2D*)(fHistList.back());
	fH2->Draw(getHistOption(fH2).c_str());
	fDisplayedHist = find(fHistList.begin(), fHistList.end(), fH2);
	PixTest::update();

	//set PG to default and clean everything:
	FinalCleaning();
	LOG(logINFO) << "PixTestPattern::doTest() done for.";
}
