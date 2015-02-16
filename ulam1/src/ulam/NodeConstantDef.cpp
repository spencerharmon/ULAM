#include <stdlib.h>
#include "NodeConstantDef.h"
#include "CompilerState.h"


namespace MFM {

  NodeConstantDef::NodeConstantDef(SymbolConstantValue * symptr, CompilerState & state) : Node(state), m_constSymbol(symptr), m_exprnode(NULL), m_currBlock(NULL), m_currBlockNo(m_state.getCurrentBlockNo())
  {
    if(symptr)
      {
	// node uses current block no, not the one saved in the symbol (e.g. pending class args)
	m_cid = symptr->getId();
      }
    else
      m_cid = 0; //error
  }

  NodeConstantDef::NodeConstantDef(const NodeConstantDef& ref) : Node(ref), m_constSymbol(NULL), m_currBlock(NULL), m_currBlockNo(ref.m_currBlockNo), m_cid(ref.m_cid)
  {
    if(ref.m_exprnode)
      m_exprnode = ref.m_exprnode->instantiate();
    else
      m_exprnode = NULL;
  }

  NodeConstantDef::~NodeConstantDef()
  {
    delete m_exprnode;
    m_exprnode = NULL;
  }

  Node * NodeConstantDef::instantiate()
  {
    return new NodeConstantDef(*this);
  }

  void NodeConstantDef::updateLineage(NNO pno)
  {
    setYourParentNo(pno);
    m_currBlock = m_state.m_currentBlock; //do it now
    assert(m_state.getCurrentBlockNo() == m_currBlockNo);
    m_exprnode->updateLineage(getNodeNo());
  }//updateLineage

  bool NodeConstantDef::findNodeNo(NNO n, Node *& foundNode)
  {
    if(Node::findNodeNo(n, foundNode))
      return true;
    return m_exprnode->findNodeNo(n, foundNode);
  } //findNodeNo

  void NodeConstantDef::printPostfix(File * fp)
  {
    m_exprnode->printPostfix(fp);
    fp->write(" = ");
    fp->write(getName());
    fp->write(" const");
  }

  const char * NodeConstantDef::getName()
  {
    assert(m_constSymbol);
    return m_state.m_pool.getDataAsString(m_constSymbol->getId()).c_str();
  }

  const std::string NodeConstantDef::prettyNodeName()
  {
    return nodeName(__PRETTY_FUNCTION__);
  }

  bool NodeConstantDef::getSymbolPtr(Symbol *& symptrref)
  {
    symptrref = m_constSymbol;
    return true;
  }

  void NodeConstantDef::setSymbolPtr(SymbolConstantValue * cvsymptr)
  {
    assert(cvsymptr);
    m_constSymbol = cvsymptr;
    m_currBlockNo = cvsymptr->getBlockNoOfST();
    assert(m_currBlockNo);
  } //setSymbolPtr

  u32 NodeConstantDef::getSymbolId()
  {
    return m_cid;
  }

  UTI NodeConstantDef::checkAndLabelType()
  {
    UTI it = Nav;
    // instantiate, look up in current block
    if(m_constSymbol == NULL)
      {
	NodeBlock * savecurrentblock = m_state.m_currentBlock; //**********
	//in case of a cloned unknown
	if(m_currBlock == NULL)
	  setBlock();

	NodeBlockClass * savememberclassblock = m_state.m_currentMemberClassBlock;
	bool saveUseMemberBlock = m_state.m_useMemberBlock;
	m_state.m_useMemberBlock = false;

	m_state.m_currentBlock = m_currBlock; //before lookup

	Symbol * asymptr = NULL;
	if(m_state.alreadyDefinedSymbol(m_cid, asymptr))
	  {
	    if(asymptr->isConstant())
	      {
		m_constSymbol = (SymbolConstantValue *) asymptr;
	      }
	    else
	      {
		std::ostringstream msg;
		msg << "(1) <" << m_state.m_pool.getDataAsString(m_cid).c_str() << "> is not a constant, and cannot be used as one";
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	      }
	  }
	else
	  {
	    std::ostringstream msg;
	    msg << "(2) Named Constant <" << m_state.m_pool.getDataAsString(m_cid).c_str() << "> is not defined, and cannot be used";
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	  }
      	m_state.m_currentBlock = savecurrentblock; //restore
	m_state.m_useMemberBlock = saveUseMemberBlock;
	m_state.m_currentMemberClassBlock = savememberclassblock;
      } //toinstantiate

    assert(m_exprnode);
    it = m_exprnode->checkAndLabelType();
    if(!(it == m_state.getUlamTypeOfConstant(Int) || it == m_state.getUlamTypeOfConstant(Unsigned)))
      {
	std::ostringstream msg;
	msg << "Constant value expression for: " << m_state.m_pool.getDataAsString(m_cid).c_str() << ", has an invalid type: <" << m_state.getUlamTypeNameByIndex(it) << ">";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	it = Nav;
      }
    setNodeType(it);
    return getNodeType();
  } //checkAndLabelType

  void NodeConstantDef::countNavNodes(u32& cnt)
  {
    Node::countNavNodes(cnt);
    m_exprnode->countNavNodes(cnt);
  }

  NNO NodeConstantDef::getBlockNo()
  {
    return m_currBlockNo;
  }

  void NodeConstantDef::setBlockNo(NNO n)
  {
    m_currBlockNo = n;
  }

  void NodeConstantDef::setBlock()
  {
    assert(m_currBlockNo);
    m_currBlock = (NodeBlock *) m_state.findNodeNoInThisClass(m_currBlockNo);
    assert(m_currBlock);
  }

  void NodeConstantDef::setBlock(NodeBlock * currblock)
  {
    assert(currblock);
    assert(m_currBlockNo == currblock->getNodeNo());
    m_currBlock = currblock;
  }

  void NodeConstantDef::setConstantExpr(Node * node)
  {
    m_exprnode = node;
  }

  // called during parsing rhs of named constant
  // Requires a constant expression, else error;
  // (scope of eval is based on the block of const def.)
  bool NodeConstantDef::foldConstantExpression()
  {
    NodeBlock * savecurrentblock = m_state.m_currentBlock; //**********
    s32 newconst = NONREADYCONST;  //always signed?

    UTI uti = checkAndLabelType(); //find any missing symbol
    assert(m_constSymbol);
    if(m_constSymbol->isReady())
      return true;

    if((uti == m_state.getUlamTypeOfConstant(Int) || uti == m_state.getUlamTypeOfConstant(Unsigned)))
      {
	evalNodeProlog(0); //new current frame pointer
	makeRoomForNodeType(getNodeType()); //offset a constant expression
	if(m_exprnode->eval() == NORMAL)
	  {
	    UlamValue cnstUV = m_state.m_nodeEvalStack.popArg();

	    if(cnstUV.getUlamValueTypeIdx() == Nav)
	      newconst = NONREADYCONST;
	    else
	      newconst = cnstUV.getImmediateData(m_state);
	  }

	evalNodeEpilog();

	if(newconst == NONREADYCONST)
	  {
	    std::ostringstream msg;
	    msg << "Constant value expression for: " << m_state.m_pool.getDataAsString(m_constSymbol->getId()).c_str() << ", is not yet ready while compiling class: " << m_state.getUlamTypeNameByIndex(m_state.m_compileThisIdx).c_str();
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), WARN);
	    m_state.m_currentBlock = savecurrentblock; //restore
	    return false;
	  }
      }
    else
      {
	std::ostringstream msg;
	msg << "Constant value expression for: " << m_state.m_pool.getDataAsString(m_constSymbol->getId()).c_str() << ", is not a constant expression";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	m_state.m_currentBlock = savecurrentblock; //restore
	return false;
      }
    // passed
    m_constSymbol->setValue(newconst); //isReady now
    m_state.m_currentBlock = savecurrentblock; //restore
    return true;
  } //foldConstantExpression

  EvalStatus NodeConstantDef::eval()
  {
    if(m_constSymbol->isReady())
      return NORMAL;
    return ERROR;
  } //eval

  void NodeConstantDef::genCode(File * fp, UlamValue& uvpass)
  {}



} //end MFM


