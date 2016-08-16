//---------------------------------------------------------------------------
#include "XmlParser.h"
#pragma hdrstop

//---------------------------------------------------------------------------

#pragma argsused
int main(int argc, char* argv[])
{
  char * szFilename = (argc > 1) ? argv[ 1 ] : NULL;
  std::wifstream * pXmlFile = NULL;
  xml::CDocTree * pDoc = NULL;

  try
  {
    pXmlFile = new std::wifstream( szFilename );
    pDoc = new xml::CDocTree( pXmlFile );
    std::cout << std::endl << *pDoc << std::endl;
  }
  catch( std::exception & rfFault )
  {
    std::cout << rfFault.what();
  }

  delete pXmlFile;
  delete pDoc;

  return 0;
}
//---------------------------------------------------------------------------
 
