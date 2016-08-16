//---------------------------------------------------------------------------
// (c) Andrew Minenkov, 2013.
//---------------------------------------------------------------------------
// This code was written as close to ISO/IEC 14882-98 as possible.
// The file contains simple xml parser declarations.

#ifndef XmlParserH
#define XmlParserH

#include <exception>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

//---------------------------------------------------------------------------
namespace xml {
//---------------------------------------------------------------------------
// This parser limited only to unicode utf-8 files
// and implements the following grammar analysis:
//
//  XmlDocument:
//    XmlComment
//    RootNode
//    ServiceNodes RootNode
//
//  XmlComment:
//    <!-- AnyChar -->
//
//  ServiceNodes:
//    <? AnyChar ?>
//
//  RootNode:
//    TerminalNode
//    TrunkNode
//
//  TrunkNode:
//    OpenTag ChildNode-list CloseTag
//
//  ChildNode-list:
//    ChildNode
//    ChildNode-list ChildNode
//
//  ChildNode:
//    TerminalNode
//    TrunkNode
//
//  TerminalNode:
//    TagNode
//    OpenTag Content CloseTag
//
//  TagNode:
//    < TagName Attribute-list />
//
//  TagName:
//    XmlName
//
//  Attribute-list:
//    Attribute
//    Attribute-list Attribute
//
//  Attribute:
//    Nothing
//    AbuteName = AbuteValue
//
//  AbuteName:
//    XmlName
//
//  AbuteValue:
//    XmlName
//    " String "
//
//  OpenTag:
//    < TagName Attributes >
//
//  CloseTag:
//    </ TagName >
//
//  XmlName:
//    AlNumChars
//
//  String:
//    AnyChExceptQuot
//
	
	///////////////////////////////////////////////////////////////////////////
	
	struct CSubstSequence {
		static CSubstSequence * TryMake( std::wstring * pSrc )
			throw( std::bad_alloc );

		const CSubstSequence & operator >>( const std::wstring * & rfpContent ) const
			throw();				

		const CSubstSequence & operator >>( size_t & rfnSrcChar ) const
			throw();

		const CSubstSequence & operator >>( std::wstring::const_iterator & rfSrcEntry ) const
			throw();

	private:
		CSubstSequence() {}

		std::wstring m_Content;
		size_t m_nSrcChar;
		std::wstring::const_iterator m_SrcEntryItor;		
	};
	
	///////////////////////////////////////////////////////////////////////////

	struct CServiceTag {
		static CServiceTag * TryMake( std::wstring * pSrc )
			throw( std::bad_alloc );

		const CServiceTag & operator >>( const std::wstring * & rfpContent ) const
			throw();		

		const CServiceTag & operator >>( size_t & rfnSrcChar ) const
			throw();

		const CServiceTag & operator >>( std::wstring::const_iterator & rfSrcEntry ) const
			throw();
			
		private:
			CServiceTag() {}

			size_t m_nRead;
			std::wstring::const_iterator m_SrcEntryItor;
			std::wstring m_ContentStr;
	};

	///////////////////////////////////////////////////////////////////////////

  struct CXmlComment {
    static CXmlComment * TryMake( std::wstring * pSrc )
      throw( std::bad_alloc );

		const CXmlComment & operator >>( const std::wstring * & rfpContent ) const
			throw();		

    const CXmlComment & operator >>( size_t & rfnCommentChar ) const
      throw();

		const CXmlComment & operator >>( std::wstring::const_iterator & rfSrcEntry ) const
			throw();

		private:
			CXmlComment() {}

			size_t m_nRead;
			std::wstring::const_iterator m_SrcEntryItor;
			std::wstring m_ContentStr;
  };

	///////////////////////////////////////////////////////////////////////////

  struct CXmlName {
    static CXmlName * TryMake( std::wstring * pSrc ) throw( std::bad_alloc );
    std::wstring String;
  };

  struct CXmlContent {
    std::wstring String;
  };

  class CAttribute {
  public:
    static CAttribute * TryMake( std::wstring * pSrc ) throw( std::bad_alloc );

    const std::wstring * const cpName;
    const std::wstring * const cpValue;

  private:
    CAttribute()
      throw();

    CXmlName m_Name;
    std::wstring m_Value;
  };

	///////////////////////////////////////////////////////////////////////////

  class CTrunkNode {
  public:
    typedef std::vector< CTrunkNode * > TXmlLevel;
    typedef std::vector< CAttribute * > TXmlAttrSet;

    typedef TXmlAttrSet::const_iterator TXmlAttrItor;
    typedef TXmlLevel::const_iterator TXmlLevelItor;

    struct ATTR_LIST {
      TXmlAttrItor First;
      TXmlAttrItor Last;
    };

    struct NODE_LIST {
      TXmlLevelItor First;
      TXmlLevelItor Last;
    };

    virtual ~CTrunkNode()
      throw();

    // Special methods
    static CTrunkNode * TryMake( std::wstring * pSrc )
      throw( std::bad_alloc );

    // Accessors.
    const CTrunkNode & operator >>( NODE_LIST & rfLevelList ) const
      throw();

    const CTrunkNode & operator >>( ATTR_LIST & rfAttrList ) const
      throw();

    const CTrunkNode & operator >>( const CXmlName * & rfpName ) const
      throw();

		// Optimizations support
		const CTrunkNode & operator >>( size_t & rfnSrcChar ) const
      throw();

		const CTrunkNode & operator >>( std::wstring::const_iterator & rfSrcEntry ) const
			throw();

  protected:
    // Inconsistent init to consequent build up with TryMake (basement construction);
    CTrunkNode() throw();

    CXmlName m_NodeName;
    TXmlAttrSet m_AttrList;

		// Optimizations support
		size_t m_nSrcChar;
		std::wstring::const_iterator m_SrcEntryItor;

  private:
    // Disable copy construction
    CTrunkNode( const CTrunkNode & rfSrc );

    TXmlLevel m_NodeList;
  };

	///////////////////////////////////////////////////////////////////////////

  class CTerminalNode : public CTrunkNode {
  public:
    static CTerminalNode * TryMake( std::wstring * pSrc )
      throw( std::bad_alloc );

    // Accessors
    const CTerminalNode & operator >>( const CXmlContent * & rfpContent ) const
      throw();

  protected:
    // Inconsistent init to consequent build up with TryMake;
    CTerminalNode()
      throw();

    CXmlContent m_Content;

  private:
    // Disable copy construction
    CTerminalNode( const CTerminalNode & rfSrc );
  };

	///////////////////////////////////////////////////////////////////////////

  class CDocTree {
  public:
    explicit CDocTree( std::wifstream * pXmlStream )
      throw( std::logic_error, std::bad_alloc );

    CDocTree( const CTrunkNode & rfRootNode )
      throw();

    ~CDocTree()
      throw();

    const CDocTree & operator >>( const CTrunkNode * & rfpRootNode ) const
      throw();

  private:
    CDocTree();
    CDocTree( const CDocTree & rfSrc );

    CTrunkNode * m_pRootNode;
    bool m_fAllowDelete;
  };

//---------------------------------------------------------------------------
}// End of xml namespace
//---------------------------------------------------------------------------

std::ostream & operator <<( std::ostream & rfDest, xml::CDocTree & rfSrc );

//---------------------------------------------------------------------------
#endif// End of include guard
//---------------------------------------------------------------------------
