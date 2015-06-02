// -- author: Xin Shi
#ifndef PIXTESTPURDUETEST_H
#define PIXTESTPURDUETEST_H

#include "PixTest.hh"

class DLLEXPORT PixTestPurdueTest: public PixTest {
public:
  PixTestPurdueTest(PixSetup *, std::string);
  PixTestPurdueTest();
  virtual ~PixTestPurdueTest();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest(); 

private:
  
  
  ClassDef(PixTestPurdueTest, 1)

};
#endif
