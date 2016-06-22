#include <stdlib.h>
#include "NodeVarRef.h"
#include "Token.h"
#include "CompilerState.h"
#include "SymbolVariableDataMember.h"
#include "SymbolVariableStack.h"
#include "NodeIdent.h"

namespace MFM {

  NodeVarRef::NodeVarRef(SymbolVariable * sym, NodeTypeDescriptor * nodetype, CompilerState & state) : NodeVarDecl(sym, nodetype, state)
  {
    Node::setStoreIntoAble(TBOOL_HAZY);
  }

  NodeVarRef::NodeVarRef(const NodeVarRef& ref) : NodeVarDecl(ref) { }

  NodeVarRef::~NodeVarRef() { }

  Node * NodeVarRef::instantiate()
  {
    return new NodeVarRef(*this);
  }

  void NodeVarRef::updateLineage(NNO pno)
  {
    NodeVarDecl::updateLineage(pno);
  } //updateLineage

  bool NodeVarRef::findNodeNo(NNO n, Node *& foundNode)
  {
    if(NodeVarDecl::findNodeNo(n, foundNode))
      return true;
    return false;
  } //findNodeNo

  void NodeVarRef::checkAbstractInstanceErrors()
  {
    //unlike NodeVarDecl, an abstract class can be a reference!
    UTI nuti = getNodeType();
    UlamType * nut = m_state.getUlamTypeByIndex(nuti);
    if(nut->getUlamTypeEnum() == Class)
      {
	SymbolClass * csym = NULL;
	AssertBool isDefined = m_state.alreadyDefinedSymbolClass(nuti, csym);
	assert(isDefined);
	if(csym->isAbstract())
	  {
	    std::ostringstream msg;
	    msg << "Instance of Abstract Class ";
	    msg << m_state.getUlamTypeNameBriefByIndex(nuti).c_str();
	    msg << " used with reference variable symbol name '";
	    msg << getName() << "'";
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), INFO);
	  }
      }
  } //checkAbstractInstanceErrors

  //see also SymbolVariable: printPostfixValuesOfVariableDeclarations via ST.
  void NodeVarRef::printPostfix(File * fp)
  {
    NodeVarDecl::printPostfix(fp);
  } //printPostfix

  void NodeVarRef::printTypeAndName(File * fp)
  {
    UTI vuti = m_varSymbol->getUlamTypeIdx();
    UlamKeyTypeSignature vkey = m_state.getUlamKeyTypeSignatureByIndex(vuti);
    UlamType * vut = m_state.getUlamTypeByIndex(vuti);

    fp->write(" ");
    if(vut->getUlamTypeEnum() != Class)
      {
	fp->write(vkey.getUlamKeyTypeSignatureNameAndBitSize(&m_state).c_str());
	fp->write("&"); //<--the only difference!!!
      }
    else
      fp->write(vut->getUlamTypeNameBrief().c_str()); //includes any &

    fp->write(" ");
    fp->write(getName());

    s32 arraysize = m_state.getArraySize(vuti);
    if(arraysize > NONARRAYSIZE)
      {
	fp->write("[");
	fp->write_decimal(arraysize);
	fp->write("]");
      }
    else if(arraysize == UNKNOWNSIZE)
      {
	fp->write("[UNKNOWN]");
      }
  } //printTypeAndName

  const char * NodeVarRef::getName()
  {
    return NodeVarDecl::getName(); //& ??
  }

  const std::string NodeVarRef::prettyNodeName()
  {
    return nodeName(__PRETTY_FUNCTION__);
  }

  FORECAST NodeVarRef::safeToCastTo(UTI newType)
  {
    UTI nuti = getNodeType();

    //cast RHS if necessary and safe
    //insure lval is same bitsize/arraysize
    // if classes, safe to cast a subclass to any of its superclasses
    FORECAST rscr = CAST_CLEAR;
    if(UlamType::compare(nuti, newType, m_state) != UTIC_SAME)
      {
	UlamType * nut = m_state.getUlamTypeByIndex(nuti);
	UlamType * newt = m_state.getUlamTypeByIndex(newType);
	rscr = m_nodeInitExpr->safeToCastTo(nuti);

	if((nut->getUlamTypeEnum() == Class))
	  {
	    if(rscr != CAST_CLEAR)
	      {
		std::ostringstream msg;
		msg << "Incompatible class types ";
		msg << nut->getUlamTypeNameBrief().c_str();
		msg << " and ";
		msg << newt->getUlamTypeNameBrief().c_str();
		msg << " used to initalize reference '" << getName() <<"'";
		if(rscr == CAST_HAZY)
		  MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
		else
		  MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	      }
	  }
	else if(m_state.isAtom(nuti))
	  {
	    //atoms init from a quark ref could use .atomof
	    // "clear" when complete, not the same as "bad".
	    if(rscr != CAST_CLEAR)
	      {
		std::ostringstream msg;
		msg << "Reference atom variable " << getName() << "'s type ";
		msg << nut->getUlamTypeNameBrief().c_str();
		msg << ", and its initial value type ";
		msg << newt->getUlamTypeNameBrief().c_str();
		msg << ", are incompatible";
		if(newt->isReference() && newt->getUlamClassType() == UC_QUARK)
		  msg << "; .atomof may help";
		if(rscr == CAST_HAZY)
		  MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
		else
		  MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	      }
	  }
	else
	  {
	    //primitives must be EXACTLY the same size (for initialization);
	    // "clear" when complete, not the same as "bad".
	    if(rscr != CAST_CLEAR)
	      {
		std::ostringstream msg;
		msg << "Reference variable " << getName() << "'s type ";
		msg << nut->getUlamTypeNameBrief().c_str();
		msg << ", and its initial value type ";
		msg << newt->getUlamTypeNameBrief().c_str();
		msg << ", are incompatible";
		if(rscr == CAST_HAZY)
		  MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
		else
		  MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	      }
	    else
	      {
		// safecast doesn't test for same size primitives since safe for op equal
		// atom sizes not related to element sizes internally.
		if((nut->getBitSize() != newt->getBitSize()) || (nut->getTotalBitSize() != newt->getTotalBitSize()))
		  {
		    std::ostringstream msg;
		    msg << "Reference variable " << getName() << "'s type ";
		    msg << nut->getUlamTypeNameBrief().c_str();
		    msg << ", and its initial value type ";
		    msg << newt->getUlamTypeNameBrief().c_str();
		    msg << ", are incompatible sizes";
		    if(m_state.isAtom(nuti) || m_state.isAtom(newType))
		      MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG);
		    else
		      {
			MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
			rscr = CAST_BAD;
		      }
		  }
	      }
	  }
      }
    //okay to explicitly cast rhs to reference type, e.g. if(a is Foo) QW& qref = (Foo &) a;
    return rscr;
  } //safeToCastTo

  UTI NodeVarRef::checkAndLabelType()
  {
    UTI it = NodeVarDecl::checkAndLabelType();
    u32 errCount= 0;
    u32 hazyCount = 0;
    if(!m_state.okUTItoContinue(it))
      {
	if(it == Nav)
	  errCount++;
	if(it == Hzy)
	  hazyCount++;
	assert(it != Nouti);
      }
    else if((!m_state.getUlamTypeByIndex(it)->isReference()))
      {
	std::ostringstream msg;
	msg << "Variable reference '";
	msg << m_state.m_pool.getDataAsString(m_vid).c_str();
	msg << "', is invalid";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	errCount++;
      }

    ////requires non-constant, non-funccall value
    //NOASSIGN REQUIRED (e.g. for function parameters) doesn't have to have this!
    if(m_nodeInitExpr)
      {
	UTI eit = m_nodeInitExpr->getNodeType();
	if(eit == Nav)
	  {
	    std::ostringstream msg;
	    msg << "Storage expression for: ";
	    msg << m_state.m_pool.getDataAsString(m_vid).c_str();
	    msg << ", is invalid";
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	    errCount++;
	  }

	if(eit == Hzy)
	  {
	    std::ostringstream msg;
	    msg << "Storage expression for: ";
	    msg << m_state.m_pool.getDataAsString(m_vid).c_str();
	    msg << ", is not ready";
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), WAIT); //possibly still hazy
	    hazyCount++;
	  }

	if(eit == Nouti)
	  {
	    std::ostringstream msg;
	    msg << "Storage expression for: ";
	    msg << m_state.m_pool.getDataAsString(m_vid).c_str();
	    msg << ", is not defined";
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), DEBUG); //possibly still hazy
	    hazyCount++;
	  }

	if(errCount)
	  {
	    setNodeType(Nav);
	    return Nav; //short-circuit
	  }

	if(hazyCount)
	  {
	    m_state.setGoAgain();
	    setNodeType(Hzy);
	    return Hzy; //short-circuit
	  }

	//check isStoreIntoAble, before any casting (i.e. named constants, func calls, NOT).
	TBOOL istor = m_nodeInitExpr->getStoreIntoAble();
	Node::setStoreIntoAble(istor); //before setReferenceAble is set

	TBOOL isrefable = m_nodeInitExpr->getReferenceAble();
	Node::setReferenceAble(isrefable); //custom arrays may have different stor/ref status

	if((isrefable != TBOOL_TRUE) || (istor != TBOOL_TRUE))
	  {
	    //error tests: t3629, t3660, t3661, t3665, t3785
	    std::ostringstream msg;
	    msg << "Initialization for: ";
	    msg << m_state.m_pool.getDataAsString(m_vid).c_str();
	    msg << ", must be referenceable"; //e.g. constant or function call NOT so
	    if(istor == TBOOL_HAZY)
	      {
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), WAIT);
		m_state.setGoAgain();
		it = Hzy;
	      }
	    else
	      {
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
		setNodeType(Nav);
		return Nav; //short-circuit
	      }
	  }

	// Don't Go Beyond This Point when either it or eit are not ok.
	if(UlamType::compareForMakingCastingNode(eit, it, m_state) == UTIC_NOTSAME)
	  {
	    //must be safe to case (NodeVarDecl c&l) is different
	    if(!Node::makeCastingNode(m_nodeInitExpr, it, m_nodeInitExpr))
	      {
		setNodeType(Nav);
		return Nav; //short-circuit
	      }
	  }
      }
    else
      {
	if(it == Nav)
	  Node::setStoreIntoAble(TBOOL_FALSE);
	else if(it == Hzy)
	  Node::setStoreIntoAble(TBOOL_HAZY);
	else
	  Node::setStoreIntoAble(TBOOL_TRUE);
      }

    setNodeType(it);
    return getNodeType();
  } //checkAndLabelType

  void NodeVarRef::packBitsInOrderOfDeclaration(u32& offset)
  {
    assert(0); //refs can't be data members
  } //packBitsInOrderOfDeclaration

  void NodeVarRef::calcMaxDepth(u32& depth, u32& maxdepth, s32 base)
  {
    return NodeVarDecl::calcMaxDepth(depth, maxdepth, base);
  } //calcMaxDepth

  void NodeVarRef::countNavHzyNoutiNodes(u32& ncnt, u32& hcnt, u32& nocnt)
  {
    NodeVarDecl::countNavHzyNoutiNodes(ncnt, hcnt, nocnt);
  } //countNavNodes

  EvalStatus NodeVarRef::eval()
  {
    assert(m_varSymbol);

    assert(m_varSymbol->getAutoLocalType() != ALT_AS); //NodeVarRefAuto

    UTI nuti = getNodeType();
    if(nuti == Nav)
      return ERROR;

    assert(m_varSymbol->getUlamTypeIdx() == nuti);
    assert(m_nodeInitExpr);

    evalNodeProlog(0); //new current frame pointer

    makeRoomForSlots(1); //always 1 slot for ptr

    EvalStatus evs = m_nodeInitExpr->evalToStoreInto();
    if(evs != NORMAL)
      {
	evalNodeEpilog();
	return evs;
      }

    UlamValue pluv = m_state.m_nodeEvalStack.popArg();
    assert(m_state.isPtr(pluv.getUlamValueTypeIdx()));
    ((SymbolVariableStack *) m_varSymbol)->setAutoPtrForEval(pluv); //for future ident eval uses
    ((SymbolVariableStack *) m_varSymbol)->setAutoStorageTypeForEval(m_nodeInitExpr->getNodeType()); //for future virtual function call eval uses

    m_state.m_funcCallStack.storeUlamValueInSlot(pluv, ((SymbolVariableStack *) m_varSymbol)->getStackFrameSlotIndex()); //doesn't seem to matter..

    evalNodeEpilog();
    return NORMAL;
  } //eval

  EvalStatus NodeVarRef::evalToStoreInto()
  {
    evalNodeProlog(0); //new current node eval frame pointer

    // (from NodeIdent's makeUlamValuePtr)
    // return ptr to this data member within the m_currentObjPtr
    // 'pos' modified by this data member symbol's packed bit position
    u32 pos = 0;
    if(m_varSymbol->isDataMember())
      pos = ((SymbolVariableDataMember *) m_varSymbol)->getPosOffset();
    UlamValue rtnUVPtr = UlamValue::makePtr(m_state.m_currentObjPtr.getPtrSlotIndex(), m_state.m_currentObjPtr.getPtrStorage(), getNodeType(), m_state.determinePackable(getNodeType()), m_state, m_state.m_currentObjPtr.getPtrPos() + pos, m_varSymbol->getId());

    //copy result UV to stack, -1 relative to current frame pointer
    Node::assignReturnValuePtrToStack(rtnUVPtr);

    evalNodeEpilog();
    return NORMAL;
  } //evalToStoreInto

  void NodeVarRef::genCode(File * fp, UVPass & uvpass)
  {
    //reference always has initial value, unless func param
    assert(m_varSymbol->isAutoLocal());
    assert(m_varSymbol->getAutoLocalType() != ALT_AS);

    if(m_nodeInitExpr)
      {
	// get the right-hand side, stgcos
	// can be same type (e.g. element, quark, or primitive),
	// or ancestor quark if a class.
	m_nodeInitExpr->genCodeToStoreInto(fp, uvpass);

	return Node::genCodeReferenceInitialization(fp, uvpass, m_varSymbol);
      }

#if 0
	UTI vuti = m_varSymbol->getUlamTypeIdx(); //i.e. this ref node
	UlamType * vut = m_state.getUlamTypeByIndex(vuti);

	if(m_state.isAtom(vuti))
	  return genCodeAtomRefInit(fp, uvpass); //splits off before array check

	Symbol * cos = NULL;
	Symbol * stgcos = NULL;
	if(m_state.m_currentObjSymbolsForCodeGen.empty())
	  {
	    stgcos = cos = m_state.getCurrentSelfSymbolForCodeGen();
	  }
	else
	  {
	    stgcos = m_state.m_currentObjSymbolsForCodeGen[0];
	    cos = m_state.m_currentObjSymbolsForCodeGen.back();
	  }

	UTI stgcosuti = stgcos->getUlamTypeIdx();
	UlamType * stgcosut = m_state.getUlamTypeByIndex(stgcosuti);

	if(!stgcosut->isScalar() && !vut->isScalar())
	  return genCodeArrayRefInit(fp, uvpass);

	if(!stgcosut->isScalar() && vut->isScalar())
	  return genCodeArrayItemRefInit(fp, uvpass);

	UTI cosuti = cos->getUlamTypeIdx();
	UlamType * cosut = m_state.getUlamTypeByIndex(cosuti);
	ULAMTYPE cosetyp = cosut->getUlamTypeEnum();

	ULAMCLASSTYPE vclasstype = vut->getUlamClassType();
	ULAMTYPE vetyp = vut->getUlamTypeEnum();

	assert(vetyp == cosetyp);
	u32 pos = 0;

	m_state.indentUlamCode(fp);
	fp->write(vut->getLocalStorageTypeAsString().c_str()); //for C++ local vars, ie non-data members
	fp->write(" ");

	fp->write(m_varSymbol->getMangledName().c_str());
	fp->write("("); //pass ref in constructor (ref's not assigned with =)
	if(stgcos->isDataMember()) //can't be an element or atom (JUST WAIT UNTIL Transients!!)
	  {
	    pos = Node::calcPosOfCurrentObjects();

	    fp->write(m_state.getHiddenArgName());
	    fp->write(", ");
	    fp->write_decimal_unsigned(pos); //rel offset
	    fp->write("u");

	    //t.f. cos must be a data member too; effective self is that of the dm;
	    // neither can be references.
	    if(cosetyp == Class)
	      {
		if(cosut->getUlamClassType() == UC_ELEMENT) //in case of transient
		  fp->write(" + T::ATOM_FIRST_STATE_BIT");
		fp->write(", &");
		fp->write(m_state.getEffectiveSelfMangledNameByIndex(cosuti).c_str());
	      }
	    else if(m_state.isAtom(cosuti)) //in case of transient
	      {
		fp->write(", uc");
	      }
	  }
	else
	  {
	    //local stg
	    if(!m_state.m_currentObjSymbolsForCodeGen.empty())
	      {
		fp->write(stgcos->getMangledName().c_str()); //even if self
		if(cos->isDataMember())
		  {
		    pos = Node::calcPosOfCurrentObjects();

		    fp->write(", ");
		    fp->write_decimal_unsigned(pos); //rel offset
		    fp->write("u");

		    //effective self is that of the data member; can't be a reference
		    if(cosetyp == Class)
		      {
			if(cosut->getUlamClassType() == UC_ELEMENT) //in case of transient stg
			  fp->write(" + T::ATOM_FIRST_STATE_BIT");
			fp->write(", &");
			fp->write(m_state.getEffectiveSelfMangledNameByIndex(cosuti).c_str());
		      }
		    else if(m_state.isAtom(cosuti)) //in case of transient stg
		      {
			fp->write(", uc");
		      }
		  }
		else
		  {
		    //cos is not a dm; neither is stgcos; t.f must be the same
		    assert(cos == stgcos); //confirm understanding

		    if(m_state.isAtom(vuti))
		      fp->write(", uc");
		    else if(vclasstype == UC_NOTACLASS)
		      {
			//no longer atom-based (pos = 0). e.g. t3617
			fp->write(", ");
			fp->write_decimal_unsigned(pos); //right-justified
			fp->write("u");
		      }
		    else if(vetyp == Class)
		      {
			if(m_state.isReference(stgcosuti))
			  {
			    fp->write(", 0u, "); //left-justified
			    fp->write(stgcos->getMangledName().c_str()); //stg
			    fp->write(".GetEffectiveSelf()");
			  }
			else
			  {
			    if(vclasstype == UC_ELEMENT)
			      fp->write(", T::ATOM_FIRST_STATE_BIT, ");
			    else
			      fp->write(", 0u, "); //left-justified
			    fp->write("&");
			    fp->write(m_state.getEffectiveSelfMangledNameByIndex(stgcosuti).c_str());
			  }
		      }
		  }
	      }
	    else
	      {
		//WHY is this different than NodeFunctionCall::genCodeAnonymousReferenceArg??
		//local var (no currentObjSymbols, 1 arg since same type)
		assert(UlamType::compare(uvpass.getPassTargetType(), vuti, m_state) == UTIC_SAME);
		fp->write(uvpass.getTmpVarAsString(m_state).c_str());
	      }
	  }

	fp->write(");"); GCNL;
      } //storage
    m_state.clearCurrentObjSymbolsForCodeGen(); //clear remnant of rhs ?
#endif

  } //genCode

#if 0
// refactored to Node
  void NodeVarRef::genCodeAtomRefInit(File * fp, UVPass & uvpass)
  {
    //reference always has initial value, unless func param
    assert(m_varSymbol->isAutoLocal());
    assert(m_varSymbol->getAutoLocalType() != ALT_AS);

    assert(m_nodeInitExpr);

    UTI vuti = m_varSymbol->getUlamTypeIdx(); //i.e. this ref node
    UlamType * vut = m_state.getUlamTypeByIndex(vuti);

    UTI puti = uvpass.getPassTargetType();

    //not necessarily if rhs is an unpacked array of atoms
    //assert(m_state.isAtom(vuti) && m_state.isAtom(puti)); //e.g. t3709 (aref = s[9])

    m_state.indentUlamCode(fp);
    fp->write(vut->getLocalStorageTypeAsString().c_str()); //for C++ local vars, ie non-data members
    fp->write(" ");

    fp->write(m_varSymbol->getMangledName().c_str());
    fp->write("("); //pass ref in constructor (ref's not assigned with =)
    if(m_state.isAtom(puti))
      {
	//both atom refs (e.g. t3692, 3671)
	if(!m_state.m_currentObjSymbolsForCodeGen.empty())
	  {
	    Symbol * stgcos = m_state.m_currentObjSymbolsForCodeGen[0];
	    fp->write(stgcos->getMangledName().c_str()); //stg
	  }
	else
	  {
	    fp->write(uvpass.getTmpVarAsString(m_state).c_str());
	  }

	if(!m_state.isAtomRef(puti))
	  {
	    fp->write(", ");
	    fp->write_decimal_unsigned(uvpass.getPassPos()); //Sun Jun 19 08:42:01 2016 ?
	    fp->write("u, uc"); //t3671
	  }
	//else, default copy constructor atomref to atomref, no uc
      }
    else
      {
	//not another atom ref, t.f. an element
	if(m_state.m_currentObjSymbolsForCodeGen.empty())
	  {
	    //does this make sense for array??? error?
	    fp->write("ur.CreateAtom()"); //need non-const T
	  }
	else
	  {
	    Symbol * stgcos = m_state.m_currentObjSymbolsForCodeGen[0];
	    UTI stgcosuti = stgcos->getUlamTypeIdx();
	    UlamType * stgcosut = m_state.getUlamTypeByIndex(stgcosuti);
	    fp->write(stgcos->getMangledName().c_str());
	    if(!m_state.isScalar(vuti))
	      {
		fp->write(", ");
		if(stgcos->isDataMember())
		  fp->write_decimal_unsigned(uvpass.getPassPos()); //t3671?
		else
		  fp->write_decimal_unsigned(0);
		fp->write("u");
	      }
	    else if(!stgcosut->isScalar())
	      {
		fp->write(", ");
		fp->write(uvpass.getTmpVarAsString(m_state).c_str());
		fp->write(" * EC::ATOM_CONFIG::BITS_PER_ATOM");
	      }
	  }
	if(m_state.isScalar(vuti))
	  fp->write(", uc");
      }

    fp->write(");"); GCNL;

    m_state.clearCurrentObjSymbolsForCodeGen(); //clear remnant of rhs ?
  } //genCodeAtomRefInit
#endif


#if 0
  void NodeVarRef::genCodeArrayRefInit(File * fp, UVPass & uvpass)
  {
    //reference always has initial value, unless func param
    assert(m_varSymbol->isAutoLocal());
    assert(m_varSymbol->getAutoLocalType() != ALT_AS);
    assert(m_nodeInitExpr);

    // get the right-hand side, stgcos
    // can be same type (e.g. element, quark, or primitive),
    // or ancestor quark if a class.
    assert(!m_state.m_currentObjSymbolsForCodeGen.empty());
    Symbol * stgcos = m_state.m_currentObjSymbolsForCodeGen[0];
    UTI stgcosuti = stgcos->getUlamTypeIdx();
    UlamType * stgcosut = m_state.getUlamTypeByIndex(stgcosuti);

    Symbol * cos = m_state.m_currentObjSymbolsForCodeGen.back();
    UTI cosuti = cos->getUlamTypeIdx();
    UlamType * cosut = m_state.getUlamTypeByIndex(cosuti);

    UTI vuti = m_varSymbol->getUlamTypeIdx(); //i.e. this ref node
    UlamType * vut = m_state.getUlamTypeByIndex(vuti);
    assert(!vut->isScalar()); //entire array

    ULAMCLASSTYPE vclasstype = vut->getUlamClassType();
    ULAMTYPE vetyp = vut->getUlamTypeEnum();
    assert(vetyp == cosut->getUlamTypeEnum());

    m_state.indentUlamCode(fp);
    fp->write(vut->getLocalStorageTypeAsString().c_str()); //for C++ local vars, ie non-data members
    fp->write(" ");

    fp->write(m_varSymbol->getMangledName().c_str());
    fp->write("("); //pass ref in constructor (ref's not assigned with =)

    if(stgcos->isDataMember()) //can't be an element or atom; UNLESS stg is a transient
      {
	fp->write(m_state.getHiddenArgName());
	fp->write(", ");
	fp->write_decimal_unsigned(uvpass.getPassPos()); //relative off ?
	fp->write("u");

	if(vetyp == Class)
	  {
	    if(vclasstype == UC_ELEMENT)
	      fp->write(" + T::ATOM_FIRST_STATE_BIT"); //?
	    if(cosut->isReference())
	      {
		fp->write(", ");
		fp->write(cos->getMangledName().c_str());
		fp->write(".GetEffectiveSelf()"); //?
	      }
	    else
	      {
		fp->write(", &");
		fp->write(m_state.getEffectiveSelfMangledNameByIndex(cosuti).c_str());
	      }
	  }
      }
    else
      {
	//local stg
	fp->write(stgcos->getMangledName().c_str());
	if(cos->isDataMember())
	  {
	    fp->write(", ");
	    fp->write_decimal_unsigned(uvpass.getPassPos()); //rel offset ?
	    fp->write("u");

	    if(vetyp == Class)
	      {
		if(cosut->getUlamClassType() == UC_ELEMENT) //in case of transient stg
		  fp->write(" + T::ATOM_FIRST_STATE_BIT");
		fp->write(", &");
		fp->write(m_state.getEffectiveSelfMangledNameByIndex(cosuti).c_str());
	      }
	    else if(m_state.isAtom(cosuti)) //in case of transient stg
	      {
		fp->write(", uc");
	      }
	  }
	else
	  {
	    // unpack and packed array use same code, except for Atom
	    if((vclasstype == UC_NOTACLASS) && (vetyp != UAtom) )
	      {
		fp->write(", 0u"); //non-atomic primitive (e.g. t3666)
	      }
	    else if((vetyp == UAtom))
	      {
		fp->write(", 0u, uc"); //Sun Jun 19 08:52:04 2016
	      }
	    else if(vetyp == Class)
	      {
		if(!stgcosut->isReference())
		  {
		    fp->write(", 0u, &"); //origin (t3670)
		    fp->write(m_state.getEffectiveSelfMangledNameByIndex(cosuti).c_str());
		  }
	      }
	    else
	      assert(0);
	  }
      } //storage
    fp->write(");"); GCNL;
    m_state.clearCurrentObjSymbolsForCodeGen(); //clear remnant of rhs ?
  } //genCodeArrayRefInit
#endif

#if 0
  void NodeVarRef::genCodeArrayItemRefInit(File * fp, UVPass & uvpass)
  {
    //reference always has initial value, unless func param
    assert(m_varSymbol->isAutoLocal());
    assert(m_varSymbol->getAutoLocalType() != ALT_AS);
    assert(m_nodeInitExpr);

    // get the right-hand side, stgcos
    // can be same type (e.g. element, quark, or primitive),
    // or ancestor quark if a class.
    assert(!m_state.m_currentObjSymbolsForCodeGen.empty());
    Symbol * stgcos = m_state.m_currentObjSymbolsForCodeGen[0];
    UTI stgcosuti = stgcos->getUlamTypeIdx();
    UlamType * stgcosut = m_state.getUlamTypeByIndex(stgcosuti);

    Symbol * cos = m_state.m_currentObjSymbolsForCodeGen.back();
    UTI cosuti = cos->getUlamTypeIdx();
    UlamType * cosut = m_state.getUlamTypeByIndex(cosuti);

    UTI vuti = m_varSymbol->getUlamTypeIdx(); //i.e. this ref node
    UlamType * vut = m_state.getUlamTypeByIndex(vuti);
    assert(vut->isScalar()); //item of array

    ULAMCLASSTYPE vclasstype = vut->getUlamClassType();
    ULAMTYPE vetyp = vut->getUlamTypeEnum();
    assert(vetyp == cosut->getUlamTypeEnum());

    u32 pos = 0;

    m_state.indentUlamCode(fp);
    fp->write(vut->getLocalStorageTypeAsString().c_str()); //for C++ local vars, ie non-data members
    fp->write(" ");

    fp->write(m_varSymbol->getMangledName().c_str());
    fp->write("("); //pass ref in constructor (ref's not assigned with =)
    if(stgcos->isDataMember()) //can't be an element or atom, unless we're in a TRANSIENT!!
      {
	pos = Node::calcPosOfCurrentObjects();

	fp->write(m_state.getHiddenArgName());
	fp->write(", ");
	if((vclasstype == UC_ELEMENT) && !m_state.isReference(cosuti))
	  fp->write("T::ATOM_FIRST_STATE_BIT + "); //need test!
	//fp->write_decimal_unsigned(((SymbolVariableDataMember *) cos)->getPosOffset()); //relative off
	fp->write_decimal_unsigned(pos); //rel offset
	fp->write("u + (");
	fp->write(uvpass.getTmpVarAsString(m_state).c_str());
	fp->write(" * ");
	fp->write_decimal_unsigned(stgcosut->getBitSize());
	fp->write("u)");
      }
    else
      {
	//local stg
	fp->write(stgcos->getMangledName().c_str());
	if(cos->isDataMember())
	  {
	    pos = Node::calcPosOfCurrentObjects();

	    fp->write(", ");
	    if(vclasstype == UC_ELEMENT)
	      fp->write("T::ATOM_FIRST_STATE_BIT + "); //need test!

	    //fp->write_decimal_unsigned(((SymbolVariableDataMember *) cos)->getPosOffset()); //relative off
	    fp->write_decimal_unsigned(pos); //rel offset
	    fp->write("u + (");
	    fp->write(uvpass.getTmpVarAsString(m_state).c_str());
	    fp->write(" * ");
	    fp->write_decimal_unsigned(stgcosut->getBitSize());
	    fp->write("u)");
	  }
	else
	  {
	    //cos is not a data member; neither is stgcos; t.f. must be the same.
	    assert(cos == stgcos); //confirm understanding.
	    if((vclasstype == UC_NOTACLASS) && (vetyp != UAtom) )
	      {
		fp->write(", (");
		fp->write(uvpass.getTmpVarAsString(m_state).c_str());
		fp->write(" * ");
		fp->write_decimal_unsigned(cosut->getBitSize());
		fp->write("u)"); //relative t3651
	      }
	    else if((vetyp == UAtom))
	      {
		fp->write(", (");
		fp->write(uvpass.getTmpVarAsString(m_state).c_str());
		fp->write(" * EC::ATOM_CONFIG::BITS_PER_ATOM");
		fp->write(", uc");
	      }
	    else if(vetyp == Class)
	      {
		fp->write(", (");
		if((vclasstype == UC_ELEMENT) && !m_state.isReference(cosuti))
		  fp->write("T::ATOM_FIRST_STATE_BIT + "); //need test!
		fp->write(uvpass.getTmpVarAsString(m_state).c_str());
		fp->write(" * ");
		fp->write_decimal_unsigned(stgcosut->getBitSize());
		fp->write("u)"); //t3810
	      }
	  }
      } //storage

    if((vclasstype != UC_NOTACLASS) && (vetyp != UAtom))
      {
	fp->write(", &"); //left just
	fp->write(m_state.getEffectiveSelfMangledNameByIndex(vuti).c_str());
      }
    fp->write(");"); GCNL;
    m_state.clearCurrentObjSymbolsForCodeGen(); //clear remnant of rhs ?
  } //genCodeArrayItemRefInit
#endif

} //end MFM
