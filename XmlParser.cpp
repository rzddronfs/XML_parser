//---------------------------------------------------------------------------
// (c) Andrew Minenkov, 2013.
//---------------------------------------------------------------------------
#include "XmlParser.h"
#include <memory>
#include <cstdlib>
#pragma hdrstop
#include <locale>


using namespace xml;

typedef std::auto_ptr< std::wstring > TAutoString;
typedef std::auto_ptr< xml::CTrunkNode > TAutoTrunkNode;
typedef std::auto_ptr< xml::CTerminalNode > TAutoTermNode;
typedef std::auto_ptr< xml::CXmlName > TAutoXmlName;
typedef std::auto_ptr< xml::CAttribute > TAutoAttribute;
typedef std::auto_ptr< xml::CXmlComment > TAutoXmlComment;
typedef std::auto_ptr< xml::CServiceTag > TAutoServiceTag;
typedef std::auto_ptr< xml::CSubstSequence > TAutoSubstSeq;

typedef std::wstring::iterator TMutableEntryItor;

const char * STR_UNINITILIZED_STREAM = "The given stream has not been correctly initialized";
const char * STR_INVALID_STREAM = "The given stream is not a valid XML document";

// Optimization
	// Max distance between adjacent xml constructs, such as tags.
	const size_t N_SKIP_LIMIT = 64; 

//---------------------------------------------------------------------------
std::wstring * ReplaceSubstSeqs( std::wstring * pReadSeq )
//---------------------------------------------------------------------------
	throw( std::bad_alloc )
{
	std::wstring * pResult = NULL;        
	
	while( CSubstSequence * pSubstSeq = CSubstSequence::TryMake( pReadSeq ) )
	{
		TAutoSubstSeq spSubstSeq( pSubstSeq );
					
		size_t n;
		std::wstring::const_iterator SrcEntryItor;
		const std::wstring * pSubstCont;

		*spSubstSeq >> n >> SrcEntryItor >> pSubstCont;

    TMutableEntryItor * pSrcEntryItor = ( TMutableEntryItor * )( & SrcEntryItor );

		pReadSeq->replace(
			*pSrcEntryItor,
      *pSrcEntryItor + n,
      pSubstCont->begin(),
      pSubstCont->end()
    );

		pResult = pReadSeq;
	}

	return pResult;
}

//---------------------------------------------------------------------------
CTrunkNode::CTrunkNode() throw()
//---------------------------------------------------------------------------
{

}

//---------------------------------------------------------------------------
CDocTree::~CDocTree()
//---------------------------------------------------------------------------
  throw()
{
  if( m_fAllowDelete )
    delete m_pRootNode;
}

//---------------------------------------------------------------------------
const CDocTree & CDocTree::operator >>( const CTrunkNode * & rfpRootNode ) const
//---------------------------------------------------------------------------
  throw()
{
  rfpRootNode = m_pRootNode;
  return *this;
}

//---------------------------------------------------------------------------
CDocTree::CDocTree( std::wifstream * pXmlStream )
//---------------------------------------------------------------------------
  throw( std::logic_error, std::bad_alloc ) :

  m_fAllowDelete( true )
{
  if( pXmlStream == NULL )
    throw std::logic_error( STR_UNINITILIZED_STREAM );

  pXmlStream->seekg( 0 );

  TAutoString spReadSeq( new std::wstring );
  std::wstring * pReadSeq = spReadSeq.get();
	
	while( !pXmlStream->eof() && pXmlStream->good() )
	{
		wchar_t wcThisChar = pXmlStream->get();
    pReadSeq->append( 1 /* count */, wcThisChar );
	}

	size_t n = 0;

	while( CXmlComment * pXmlComment = CXmlComment::TryMake( pReadSeq ) )
	{	
		TAutoXmlComment spXmlComment( pXmlComment );
    
		size_t nSize;
    std::wstring::const_iterator SrcEntryItor;
			
		*spXmlComment >> nSize >> SrcEntryItor;

    TMutableEntryItor * pSrcEntryItor = ( TMutableEntryItor * )( & SrcEntryItor );

    spReadSeq->erase(
      *pSrcEntryItor,
			*pSrcEntryItor + nSize
    );

		++n;
	}

	while( CServiceTag * pServiceTag = CServiceTag::TryMake( pReadSeq )  )
	{
		TAutoServiceTag spServiceTag( pServiceTag );

		size_t nSize;
    std::wstring::const_iterator SrcEntryItor;
			
		*spServiceTag >> nSize >> SrcEntryItor;

    TMutableEntryItor * pSrcEntryItor = (TMutableEntryItor *)( & SrcEntryItor );

    spReadSeq->erase(
      *pSrcEntryItor,
			*pSrcEntryItor + nSize
    ); 
	}
 
  m_pRootNode = CTerminalNode::TryMake( pReadSeq );
    
  if( m_pRootNode == NULL )
  {
    m_pRootNode = CTrunkNode::TryMake( pReadSeq );
  }

  if( m_pRootNode == NULL )
    throw std::logic_error( STR_INVALID_STREAM );
}

//---------------------------------------------------------------------------
const CXmlComment & CXmlComment::operator >>( size_t & rfnCommentChar ) const
//---------------------------------------------------------------------------
  throw()
{
  rfnCommentChar = this->m_nRead;
  return *this;
}

//---------------------------------------------------------------------------
const CXmlComment & CXmlComment::operator >>( std::wstring::const_iterator & rfSrcEntry ) const
//---------------------------------------------------------------------------
	throw()
{
	rfSrcEntry = this->m_SrcEntryItor;
	return *this;	
}

//---------------------------------------------------------------------------
const CServiceTag & CServiceTag::operator >>( size_t & rfnSrcChar ) const
//---------------------------------------------------------------------------
	throw()
{
	rfnSrcChar = this->m_nRead;
	return *this;
}

//---------------------------------------------------------------------------
const CServiceTag & CServiceTag::operator >>( const std::wstring * & rfpContent ) const
//---------------------------------------------------------------------------
	throw()
{
	rfpContent = & this->m_ContentStr;
	return *this;
}

//---------------------------------------------------------------------------
const CServiceTag & CServiceTag::operator >>( std::wstring::const_iterator & rfSrcEntry ) const
//---------------------------------------------------------------------------
	throw()
{
	rfSrcEntry = this->m_SrcEntryItor;
	return *this;	
}

//---------------------------------------------------------------------------
const CXmlComment & CXmlComment::operator >>( const std::wstring * & rfpContent ) const
//---------------------------------------------------------------------------
	throw()
{
	rfpContent = & this->m_ContentStr;
	return *this;
}

//---------------------------------------------------------------------------
const CTrunkNode & CTrunkNode::operator >>( std::wstring::const_iterator & rfSrcEntry ) const
//---------------------------------------------------------------------------
	throw()
{
	rfSrcEntry = this->m_SrcEntryItor;
	return *this;	
}

//---------------------------------------------------------------------------
CXmlComment * CXmlComment::TryMake( std::wstring * pSrc ) throw( std::bad_alloc )
//---------------------------------------------------------------------------
{
  enum STATE {
    INITIAL = 0,
    LAB, // Left Angle Bracket
    LES, // Left Exclamation Sign
    LFH, // Left First Hyphen
    LSH, // Left Second Hyphen
    RFH, // Right First Hyphen
    RSH,
    RAB,
    FINAL
  };

  CXmlComment * pResult = NULL;

  if( pSrc != NULL && pSrc->size() > 0 )
  {
    TAutoString spReadSeq( new std::wstring() );
    TAutoXmlComment spResult( new CXmlComment() );
    STATE eState = INITIAL;
    size_t nRead = 0;
		size_t nPreParse = 0;

    while( nRead < pSrc->size() && eState < FINAL )
    {
      wchar_t wcThisChar = pSrc->at( nRead );

      spReadSeq->push_back( wcThisChar );

      bool fMatch = true;
      bool fSequental = spReadSeq->size() == 1;

      if( eState == INITIAL && wcThisChar == L'<' )
      {
        nPreParse = nRead;
				spResult->m_SrcEntryItor = pSrc->begin() + nRead;
				eState = LAB;
      }
      else if( eState == LAB && wcThisChar == L'!' && fSequental )
      {
				eState = LES;
      }
      else if( eState == LES && wcThisChar == L'-' && fSequental )
      {
        eState = LFH;
      }
      else if( eState == LFH && wcThisChar == L'-' && fSequental )
      {
        eState = LSH;
      }
      else if( eState == LSH && wcThisChar == L'-' )
      {
        spReadSeq->pop_back();
        spResult->m_ContentStr.append( *spReadSeq );
        eState = RFH;
      }
      else if( eState == RFH && wcThisChar == L'-' )
      {
        if( fSequental )
        {
          eState = RSH;
        }
        else
        {
          spReadSeq->pop_back();
          spResult->m_ContentStr.append( 1 /* count*/, L'-' );
          spResult->m_ContentStr.append( *spReadSeq );
        }
      }
      else if( eState == RSH && wcThisChar == L'>' )
      {
        if( fSequental )
        {
          eState = RAB;
        }
        else
        {
          spResult->m_ContentStr.append( 2 /* count*/, L'-' );
          spResult->m_ContentStr.append( *spReadSeq );
          eState = LSH;
        }
      }
      else
        fMatch = false;

      if( fMatch )
      {
        spReadSeq->clear();
      }

			if( !fMatch && eState < LSH )
			{
				eState = INITIAL;
			}

      if( eState == RAB )
      {
        eState = FINAL;
      }

      ++nRead;
    }

		spResult->m_nRead = nRead - nPreParse;

    if( eState == FINAL )
    {
      pResult = spResult.release();
    }
  }

  return pResult;
}

//---------------------------------------------------------------------------
const CTrunkNode & CTrunkNode::operator >>( size_t & rfnSrcChar ) const
//---------------------------------------------------------------------------
	throw()
{
	rfnSrcChar = this->m_nSrcChar;
	return *this;
}

//---------------------------------------------------------------------------
CTrunkNode * CTrunkNode::TryMake( std::wstring * pSrc ) throw( std::bad_alloc )
//---------------------------------------------------------------------------
{
  enum STATE {
      INITIAL = 0,
      OTLAB, // Open Tag Left Angle Bracket
      OTNAME,
      OTATT,
      OTRAB,
      SUBNODE,
			CTSEARCH, // Close Tag Search
      CTLAB, // Close Tag Left Angle Bracket
      CTSLASH,
      CTNAME,
      CTRAB,
      FINAL
  };

  CTrunkNode * pResult = NULL;

  if( pSrc != NULL && pSrc->size() > 10 ) // <A><B/></A> => 11 chars => min valid format
  {
    size_t n = 0;
		size_t nPreParse = 0;

    TAutoString spReadSeq( new std::wstring );
    std::wstring * pReadSeq = spReadSeq.get();

    TAutoTrunkNode spResult( new CTrunkNode() );

    STATE eState = INITIAL;

    while( n < pSrc->size() && eState < FINAL )
    {
      wchar_t wcThisChar = pSrc->at( n );
      pReadSeq->append( 1 /* count */, wcThisChar );

			bool fSequental = pReadSeq->size() == 1;
			bool fMatch = true;
			bool fAttProcess = (eState == OTNAME || eState == OTATT);
			bool fSubnodeProcess = (eState == OTRAB || eState == SUBNODE);

      if( eState == INITIAL && wcThisChar == L'<' )
      {       
        nPreParse = n;
				spResult->m_SrcEntryItor = pSrc->begin() + n;
				eState = OTLAB;
      }
			else if( eState == OTLAB )
      {
        if( wcThisChar == L'/' )
				{
					n = pSrc->size();
				}
				else if( CXmlName * pName = CXmlName::TryMake( pReadSeq ) )
        {
          TAutoXmlName spName( pName );
          spResult->m_NodeName.String.swap( pName->String );
          
          eState = OTNAME;
					
					// Parse wcThisChar again since XmlName detection requires 
					// an extra character which is not in name detection set.
					--n; 
        }
				else
					fMatch = false;
      }
			else if( fAttProcess )
      {
				if( wcThisChar == L'>' )
				{      
					eState = OTRAB;
				}
				else if( CAttribute * pAttribute = CAttribute::TryMake( pReadSeq ) )
        {
          TAutoAttribute spAttribute( pAttribute );
          spResult->m_AttrList.push_back( pAttribute );
          spAttribute.release();
					        
          eState = OTATT;
        }
				else
					fMatch = false;
      }
			else if( (eState == CTSEARCH || eState == SUBNODE) && wcThisChar == L'<' )
      {       
        eState = CTLAB;
      }
			else if( fSubnodeProcess )
      {
        // The optimization excludes extra lookups on detection of parse fragment
					pReadSeq->clear();
					pReadSeq->append( pSrc->begin() + n, pSrc->end() );
				 
				CTrunkNode * pSubnode = CTerminalNode::TryMake( pReadSeq );
        if( pSubnode == NULL )
        {
          pSubnode = CTrunkNode::TryMake( pReadSeq );
        }

				// Optimization: disables successive calls of TryMake() methods above
				// when there are whitespaces between last child node and closing tag.
					eState = CTSEARCH;

        if( pSubnode != NULL )
        {
          TAutoTrunkNode spSubnode( pSubnode );
          spResult->m_NodeList.push_back( pSubnode );

					// Optimization causes recalculation of the value of n.
						std::wstring::const_iterator SrcEntryItor;
						size_t nSubnodeSize;
						*spSubnode >> SrcEntryItor >> nSubnodeSize;					
						size_t nParsed = SrcEntryItor + nSubnodeSize - pReadSeq->begin();
						n += nParsed - 1; 

          spSubnode.release();         
          eState = SUBNODE;				
        }
				// Disabled by optimization
				//else
					//fMatch = false;
      }
			else if( eState == CTLAB && wcThisChar == L'/' && fSequental )
      {       
        eState = CTSLASH;
      }
			else if( eState == CTSLASH )
      {
        if( CXmlName * pName = CXmlName::TryMake( pReadSeq ) )
        {
          TAutoXmlName spName( pName );         
          eState = CTNAME;
					--n;
        }
				else
					fMatch = false;
      }
			else if( eState == CTNAME && wcThisChar == L'>' )
      {      
        eState = CTRAB;
      }
			else
				fMatch = false;

			if( fMatch )
			{
				spReadSeq->clear();
			}

			if( eState == CTRAB )
      {
        eState = FINAL;
      }

			// Optimization
				if( n == N_SKIP_LIMIT && eState < OTNAME )
				{
					n = pSrc->size();
				}

      ++n;
    }// while

		// Optimization
			spResult->m_nSrcChar = n - nPreParse;

    if( eState == FINAL )
    {
      pResult = spResult.release();
    }
  }// if the source string present.

  return pResult;
}

//---------------------------------------------------------------------------
CTerminalNode * CTerminalNode::TryMake( std::wstring * pSrc ) throw( std::bad_alloc )
//---------------------------------------------------------------------------
{
  enum STATE {
    INITIAL = 0,
    OTLAB, // Open Tag Left Angle Bracket
    OTNAME,
    OTATTR,
    OTSLASH,
    OTRAB,
    QCONTENT, // Qouted content
    BARECONT,
    CTLAB, // Close Tag Left Angle Bracket
    CTSLASH,
    CTNAME,
    CTRAB,
    FINAL
  };

  CTerminalNode * pResult = NULL;

  if( pSrc != NULL && pSrc->size() > 3 ) // <A/> => 4 chars - min valid format.
  {
    TAutoString spReadSeq( new std::wstring );
    std::wstring * pReadSeq = spReadSeq.get();

    TAutoTermNode spResult( new CTerminalNode() );

    STATE eState = INITIAL;

		size_t nPreParse = 0;
    size_t n = 0;
    while( n < pSrc->size() && eState < FINAL )
    {
      wchar_t wcThisChar = pSrc->at( n );
      spReadSeq->append( 1 /* count */, wcThisChar );

			bool fSequental = spReadSeq->size() == 1;
			bool fAttrProcess = eState == OTNAME || eState == OTATTR;

			bool fMatch = true;

      if( eState == INITIAL && wcThisChar == L'<' )
      {
        nPreParse = n;
				spResult->m_SrcEntryItor = pSrc->begin() + n;
				
        eState = OTLAB;
      }
			else if( eState == OTLAB )
      {
        if( wcThisChar == L'/' )
				{
					n = pSrc->size();
				}
				else if( CXmlName * pNodeName = CXmlName::TryMake( pReadSeq ) )
        {
          TAutoXmlName spNodeName( pNodeName );
          spResult->m_NodeName.String.swap( pNodeName->String );
          
          eState = OTNAME;
					--n;
        }
				else
					fMatch = false;
      }
			else if( fAttrProcess )
      {
				if( wcThisChar == L'>' )
				{       
					eState = OTRAB;					

					if( pReadSeq->size() > 1 && *(pReadSeq->end() - 2) == L'/' )
						eState = FINAL;						
				}        
				else if( CAttribute * pAttribute = CAttribute::TryMake( pReadSeq ) )
        {
          TAutoAttribute spAttribute( pAttribute );
          spResult->m_AttrList.push_back( pAttribute );
          spAttribute.release();
          
          eState = OTATTR;
					--n;
        }
				else 
					fMatch = false;
      }
			else if( eState == QCONTENT && wcThisChar == L'\"' )
      {
        ReplaceSubstSeqs( pReadSeq );
				
				spResult->m_Content.String.append( *pReadSeq );       
        eState = BARECONT;
      }
			else if( eState == OTRAB )
      {        
				spResult->m_Content.String.push_back( wcThisChar );
				
				if( wcThisChar == L'\"' )
				{
					eState = QCONTENT;
				}        
				if( wcThisChar == L'<' )
				{
					eState = CTLAB;
					spResult->m_Content.String.pop_back();
				}
				else
				{												
					eState = BARECONT;
				}
	    }
			else if( eState == BARECONT && wcThisChar == L'<' )
      {
        pReadSeq->pop_back();
				ReplaceSubstSeqs( pReadSeq );
        spResult->m_Content.String.append( *pReadSeq );
       
        eState = CTLAB;
      }
			else if( eState == CTLAB && wcThisChar == L'/' && fSequental )
      {        
        eState = CTSLASH;
      }
			else if( eState == CTSLASH )
      {
        if( CXmlName * pNodeName = CXmlName::TryMake( pReadSeq ) )
        {
          TAutoXmlName spNodeName( pNodeName );          
          eState = CTNAME;
					--n;
        }
				else
					fMatch = false;
      }
			else if( eState == CTNAME && wcThisChar == L'>' )
      {      
        eState = CTRAB;
      }
			else
				fMatch = false;


      if( eState == CTRAB )
      {
        eState = FINAL;
      }

			if( fMatch )
			{
				pReadSeq->clear();
			}

			// Optimization
				if( n == N_SKIP_LIMIT && eState < OTNAME )
				{
					n = pSrc->size();
				}

      ++n;
    }// while

		// Optimization
			spResult->m_nSrcChar = n - nPreParse;

    if( eState == FINAL )
    {
      pResult = spResult.release();
    }

  }// if the source string present.

  return pResult;
}

//---------------------------------------------------------------------------
CAttribute * CAttribute::TryMake( std::wstring * pSrc ) throw( std::bad_alloc )
//---------------------------------------------------------------------------
{
  using namespace std;

  enum STATE {
    INITIAL = 0,
    ATTNAME,
    EQUSIGN,
    QSTRING,
    BARESTR,
    FINAL
  };

  CAttribute * pResult = NULL;

  if( pSrc != NULL && pSrc->size() > 0 )
  {
    TAutoAttribute spResult( new CAttribute() );

    TAutoString spReadSeq( new std::wstring );
    std::wstring * pReadSeq = spReadSeq.get();

    STATE eState = INITIAL;

    size_t n = 0;
    while( n < pSrc->size() && eState < FINAL )
    {
      wchar_t wcThisChar = pSrc->at( n );
      pReadSeq->append( 1 /* count */, wcThisChar );

			bool fMatch = true;

      if( eState == INITIAL )
      {        
				// Since XmlName parser looks for the first not name character 
				// the final state machine should continue updating of its state
				// right on this iteration.
				if( CXmlName * pAttName = CXmlName::TryMake( pReadSeq ) )
        {
          TAutoXmlName spAttName( pAttName );
					spResult->m_Name.String.swap( pAttName->String );
          eState = ATTNAME;
        }
				else
					fMatch = false;			
      }
			
			if( eState == ATTNAME && wcThisChar == L'=' )
      {
        eState = EQUSIGN;
      }
			else if( eState == EQUSIGN && wcThisChar == L'\"' )
      {
        spResult->m_Value.push_back( wcThisChar );
        eState = QSTRING;
      }
			else if( eState == QSTRING && wcThisChar == L'\"' )
      {
        ReplaceSubstSeqs( pReadSeq );
				spResult->m_Value.append( *pReadSeq );
        eState = BARESTR;
      }
      // Borland C++ 6.0 BUG: since std::isalnum template function is not correctly
      // overloaded, iswalnum() one used instead.
      else if( eState == EQUSIGN && iswalnum( wcThisChar ) )
      {
        spResult->m_Value.push_back( wcThisChar );
        eState = BARESTR;
      }
      else if( eState == BARESTR && !iswalnum( wcThisChar ) )
      {
        pReadSeq->pop_back();
        spResult->m_Value.append( *pReadSeq );
        eState = FINAL;
      }
			else
				fMatch = false;

			if( fMatch )
			{
				spReadSeq->clear();
			}

			// Optimization
				if( n == N_SKIP_LIMIT && eState < ATTNAME )
				{
					n = pSrc->size();
				}

      ++n;
    }// while

    if( eState == FINAL )
    {
      pResult = spResult.release();
    }
  }// if source string is not empty

  return pResult;
}

//---------------------------------------------------------------------------
CXmlName * CXmlName::TryMake( std::wstring * pSrc ) throw( std::bad_alloc )
//---------------------------------------------------------------------------
{
  using namespace std;

  enum STATE {
    INITIAL = 0,
    ALNUM,
    FINAL
  };

  CXmlName * pResult = NULL;

  if( pSrc != NULL && pSrc->size() > 0 )
  {
    TAutoXmlName spResult( new CXmlName() );

    TAutoString spReadSeq( new std::wstring );
    std::wstring * pReadSeq = spReadSeq.get();

    STATE eState = INITIAL;

    size_t n = 0;
    while( n < pSrc->size() && eState < FINAL )
    {
      wchar_t wcThisChar = pSrc->at( n );
      pReadSeq->push_back( wcThisChar );

      if( eState == INITIAL && iswalnum( wcThisChar ) )
      {
        spResult->String.push_back( wcThisChar );
        pReadSeq->clear();
        eState = ALNUM;
      }

      if( eState ==  ALNUM && !iswalnum( wcThisChar ) && pReadSeq->size() > 0 )
      {
        pReadSeq->pop_back();
        spResult->String.append( *pReadSeq );
        pReadSeq->clear();
        eState = FINAL;
      }

      ++n;
    }

    if( eState == FINAL )
    {
      pResult = spResult.release();
    }

  }// if source string is not empty

  return pResult;
}

//---------------------------------------------------------------------------
CTrunkNode::~CTrunkNode() throw()
//---------------------------------------------------------------------------
{
  for( size_t n = 0; n < m_AttrList.size(); ++n )
  {
    delete m_AttrList.at( n );
  }

  for( size_t n = 0; n < m_NodeList.size(); ++n )
  {
    delete m_NodeList.at( n );
  }
}

//---------------------------------------------------------------------------
CAttribute::CAttribute() throw() :
//---------------------------------------------------------------------------
  cpName( & m_Name.String ), cpValue( & m_Value )
{

}

//---------------------------------------------------------------------------
CTerminalNode::CTerminalNode() throw() :
//---------------------------------------------------------------------------
  CTrunkNode()
{

}

//---------------------------------------------------------------------------
const CTrunkNode & CTrunkNode::operator >>( NODE_LIST & rfLevelList ) const
//---------------------------------------------------------------------------
  throw()
{
  rfLevelList.First = m_NodeList.begin();
  rfLevelList.Last = m_NodeList.end();
  return *this;
}

//---------------------------------------------------------------------------
const CTerminalNode & CTerminalNode::operator >>( const CXmlContent * & rfpContent ) const
//---------------------------------------------------------------------------
  throw()
{
  rfpContent = & m_Content;
  return *this;
}

//---------------------------------------------------------------------------
const CTrunkNode & CTrunkNode::operator >>( const CXmlName * & rfpName ) const
//---------------------------------------------------------------------------
  throw()
{
  rfpName = & m_NodeName;
  return *this;
}

//---------------------------------------------------------------------------
const CTrunkNode & CTrunkNode::operator >>( ATTR_LIST & rfAttrList ) const
//---------------------------------------------------------------------------
  throw()
{
  rfAttrList.First = m_AttrList.begin();
  rfAttrList.Last = m_AttrList.end();
  return *this;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class CStrBuffer {
public:
  explicit CStrBuffer( size_t n )
  {
    szCStr = new char[ n ];
  }

  ~CStrBuffer()
  {
    delete[] szCStr;
  }

  char * szCStr;
};

//---------------------------------------------------------------------------
CDocTree::CDocTree( const CTrunkNode & rfRootNode )
//---------------------------------------------------------------------------
  throw() :

  m_fAllowDelete( false )
{
  m_pRootNode = const_cast< CTrunkNode * >( & rfRootNode );
}

//---------------------------------------------------------------------------
std::ostream & operator <<( std::ostream & rfDest, xml::CDocTree & rfSrc )
//---------------------------------------------------------------------------
{
  typedef xml::CTrunkNode::ATTR_LIST ATTR_LIST;
  typedef xml::CTrunkNode::NODE_LIST NODE_LIST;
  using namespace std;

  ATTR_LIST AttrList;
  NODE_LIST LevelList;
  const xml::CXmlName * pXmlName;
  const CTrunkNode * pRootNode = NULL;

  rfSrc >> pRootNode;
  *pRootNode >> AttrList >> LevelList >> pXmlName;

  // Name output stack frame
  {
    size_t n = pXmlName->String.length();
    CStrBuffer StrBuffer( n + 1 );

    std::wcstombs( StrBuffer.szCStr, pXmlName->String.c_str(), n + 1 );
    rfDest << "\n\n" << "Node Name : \t" << StrBuffer.szCStr << endl;
  }

  while( AttrList.First < AttrList.Last )
  {
    rfDest << endl << "Node attribute:" << endl;
    const xml::CAttribute * pAttribute = *AttrList.First;

    // Attribute name output
    {

      size_t n = pAttribute->cpName->length();
      CStrBuffer StrBuffer( n + 1 );

      std::wcstombs( StrBuffer.szCStr, pAttribute->cpName->c_str(), n + 1 );
      rfDest << "\t" << "Name : \t" << StrBuffer.szCStr << endl;
    }

    // Attribute value output
    {
      size_t n = pAttribute->cpValue->length();
      CStrBuffer StrBuffer( n + 1 );

      std::wcstombs( StrBuffer.szCStr, pAttribute->cpValue->c_str(), n + 1 );
      rfDest << "\t" << "Value : \t" << StrBuffer.szCStr << endl;
    }
    
    ++AttrList.First;
  }

  // Node content output
  if( LevelList.First == LevelList.Last  )
  {
		const xml::CTerminalNode * pThisNode = static_cast< const xml::CTerminalNode * >( pRootNode );
    const xml::CXmlContent * pNodeContent;

    *pThisNode >> pNodeContent;

    size_t n = pNodeContent->String.length();
    CStrBuffer StrBuffer( n + 1 );

    std::wcstombs( StrBuffer.szCStr, pNodeContent->String.c_str(), n + 1 );
    rfDest << "\t" << "Node Content : \t" << StrBuffer.szCStr << endl;
  }

  while( LevelList.First < LevelList.Last )
  {
    xml::CTrunkNode * pChildNode = *LevelList.First;
    CDocTree NewRoot( *pChildNode );

    rfDest << "Child node: \n" << NewRoot << endl;
    ++LevelList.First;
  }

  return rfDest;
}

//---------------------------------------------------------------------------
CServiceTag * CServiceTag::TryMake( std::wstring * pSrc )
//---------------------------------------------------------------------------
	throw( std::bad_alloc )
{
	enum STATE {
		INITIAL = 0,
		LAB, // Left Angle Bracket
		LQM, // Left Question Mark
		RQM, // Right Question Mark
		RAB, // Right Angle Bracket
		FINAL
	};

	CServiceTag * pResult = NULL;

	if( pSrc != NULL && pSrc->size() > 0 )
	{
		TAutoString spReadSeq( new std::wstring );
		std::wstring * pReadSeq = spReadSeq.get();
		size_t nPreParse = 0;
		size_t n = 0;
		STATE eState = INITIAL;
		TAutoServiceTag spResult( new CServiceTag );

		while( n < pSrc->size() && eState < FINAL )
		{
			wchar_t wcThisChar = pSrc->at( n );
			spReadSeq->append( 1 /* count */, wcThisChar );
			bool fMatch = true;
			bool fSequental = spReadSeq->size() == 1;

			if( eState == INITIAL && wcThisChar == L'<' )
			{
				nPreParse = n;
				spResult->m_SrcEntryItor = pSrc->begin() + n;
				eState = LAB;
			}
			else if( eState == LAB && wcThisChar == L'?' && fSequental )
			{
				eState = LQM;
			}
			else if( eState == LQM && wcThisChar == L'?' )
			{	
				pReadSeq->pop_back();
				spResult->m_ContentStr.append( *pReadSeq );
				eState = RQM;
			}
			else if( eState == RQM && wcThisChar == L'>' )
			{
				if( fSequental )
				{
					eState = RAB;
				}
				else
				{
					spResult->m_ContentStr.push_back( L'?' );
					spResult->m_ContentStr.append( *pReadSeq );
					eState = LQM;
				}
			}
			else
				fMatch = false;

			if( fMatch )
			{
				spReadSeq->clear();
			}
			
			if( eState == RAB )
			{
				eState = FINAL;
			}	
			
			++n;
		}// while

		spResult->m_nRead = n - nPreParse;

		if( eState == FINAL )
		{
			pResult = spResult.release();
		}
	}

	return pResult;
}

//---------------------------------------------------------------------------
const CSubstSequence & CSubstSequence::operator >>( const std::wstring * & rfpContent ) const
//---------------------------------------------------------------------------
	throw()
{
	rfpContent = & this->m_Content;
	return *this;
}

//---------------------------------------------------------------------------
CSubstSequence * CSubstSequence::TryMake( std::wstring * pSrc )
//---------------------------------------------------------------------------
	throw( std::bad_alloc )
{
	static const wchar_t * awsSamples[] = { L"\&quot\;",	L"\&lt\;", L"\&gt\;" };
	static const wchar_t awcResults[]		= { L'\"'			 ,	L'<'		 , L'>'			 };
	static const size_t nSamples = sizeof( awsSamples )/sizeof( awsSamples[0] );
	
	CSubstSequence * pResult = NULL;

	if( pSrc != NULL && pSrc->size() > 3 ) // &quot; &lt; &gt; => min 4 symbols.
	{
		TAutoSubstSeq spResult( new CSubstSequence );

		size_t n = 0;
		size_t nPos = std::wstring::npos;
		wchar_t wcResult;
		while( n < nSamples && nPos == std::wstring::npos )
		{
			nPos = pSrc->find( awsSamples[ n ] );
			spResult->m_nSrcChar = std::wcslen( awsSamples[ n ] );
			wcResult = awcResults[ n ];						
			++n;
		}

		if( nPos != std::wstring::npos )
		{
			spResult->m_Content.push_back( wcResult );
			spResult->m_SrcEntryItor = pSrc->begin() + nPos;
			pResult = spResult.release();	
		}		
	}// if

	return pResult;
}

//---------------------------------------------------------------------------
const CSubstSequence & CSubstSequence::operator >>( size_t & rfnSrcChar ) const
//---------------------------------------------------------------------------
	throw()
{
	rfnSrcChar = this->m_nSrcChar;
	return *this;
}

//---------------------------------------------------------------------------
const CSubstSequence & CSubstSequence::operator >>( std::wstring::const_iterator & rfSrcEntry ) const
//---------------------------------------------------------------------------
	throw()
{
	rfSrcEntry = this->m_SrcEntryItor;
	return *this;
}
