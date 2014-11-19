#include "TestCase_EndToEndParser.h"

namespace MFM {

  BEGINTESTCASEPARSER(t214_test_parser_boolarrays)
  {
    std::string GetAnswerKey()
    {
      return std::string("Ue_A { Bool(1) a[5](false,false,false,false,true);  Int(32) test() {  a 1 3 +b [] true cast = 0 cast return } }\n");
    }
    
    std::string PresetTest(FileManagerString * fms)
    {
      bool rtn1 = fms->add("A.ulam","element A {\n Bool a[5];\n Int test() {\n a[1+3] = true;\n return 0;\n}\n }\n");
      
      if(rtn1)
	return std::string("A.ulam");
      
      return std::string("");
    }      
  }
  
  ENDTESTCASEPARSER(t214_test_parser_boolarrays)
  
} //end MFM


