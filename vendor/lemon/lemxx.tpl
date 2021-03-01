/*

January, 2016.  Sample class-based version.
#define LEMON_SUPER as the name of a class which overrides lemon_base<TokenType>.  
The parser will be implemented in terms of that.
add a %code section to instantiate it.
 */

/*
** 2000-05-29
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Driver template for the LEMON parser generator.
**
** The "lemon" program processes an LALR(1) input grammar file, then uses
** this template to construct a parser.  The "lemon" program inserts text
** at each "%%" line.  Also, any "P-a-r-s-e" identifer prefix (without the
** interstitial "-" characters) contained in this template is changed into
** the value of the %name directive from the grammar.  Otherwise, the content
** of this template is copied straight through into the generate parser
** source file.
**
** The following is the concatenation of all %include directives from the
** input grammar file:
*/
#include <cassert>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>

namespace {

  // use std::allocator etc?


  // this is here so you can do something like Parse(void *, int, my_token &&) or (... const my_token &)
  template<class T> struct yy_fix_type {
    typedef typename std::remove_const<typename std::remove_reference<T>::type>::type type;
  };

  template<>
  struct yy_fix_type<void> {
    typedef struct {} type;
  };

  template<class T, class... Args>
  typename yy_fix_type<T>::type &yy_constructor(void *vp, Args&&... args ) {
    typedef typename yy_fix_type<T>::type TT;
    TT *tmp = ::new(vp) TT(std::forward<Args>(args)...);
    return *tmp;
  }


  template<class T>
  typename yy_fix_type<T>::type &yy_cast(void *vp) {
    typedef typename yy_fix_type<T>::type TT;
    return *(TT *)vp;
  }


  template<class T>
  void yy_destructor(void *vp) {
    typedef typename yy_fix_type<T>::type TT;
    ((TT *)vp)->~TT();
  }


  template<class T>
  void yy_destructor(T &t) {
    t.~T();
  }



  template<class T>
  void yy_move(void *dest, void *src) {
    typedef typename yy_fix_type<T>::type TT;

    TT &tmp = yy_cast<TT>(src);
    yy_constructor<TT>(dest, std::move(tmp));
    yy_destructor(tmp);
  }


  // this is to destruct references in the event of an exception.
  // only the LHS needs to be deleted -- other items remain on the 
  // shift/reduce stack in a valid state 
  // (as long as the destructor) doesn't throw!
  template<class T>
  struct yy_auto_deleter {

    yy_auto_deleter(T &t) : ref(t), enaged(true)
    {}
    yy_auto_deleter(const yy_auto_deleter &) = delete;
    yy_auto_deleter(yy_auto_deleter &&) = delete;
    yy_auto_deleter &operator=(const yy_auto_deleter &) = delete;
    yy_auto_deleter &operator=(yy_auto_deleter &&) = delete;

    ~yy_auto_deleter() {
      if (enaged) yy_destructor(ref);
    }
    void cancel() { enaged = false; }

  private:
    T& ref;
    bool enaged=false;
  };

  template<class T>
  class yy_storage {
  private:
    typedef typename yy_fix_type<T>::type TT;

  public:
    typedef typename std::conditional<
      std::is_trivial<TT>::value,
      TT,
      typename std::aligned_storage<sizeof(TT),alignof(TT)>::type
    >::type type;
  };

}

/************ Begin %include sections from the grammar ************************/
%%
/**************** End of %include directives **********************************/
/* These constants specify the various numeric values for terminal symbols
** in a format understandable to "makeheaders".  This section is blank unless
** "lemon" is run with the "-m" command-line option.
***************** Begin makeheaders token definitions *************************/
%%
/**************** End makeheaders token definitions ***************************/

/* The next sections is a series of control #defines.
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used to store the integer codes
**                       that represent terminal and non-terminal symbols.
**                       "unsigned char" is used if there are fewer than
**                       256 symbols.  Larger types otherwise.
**    YYNOCODE           is a number of type YYCODETYPE that is not used for
**                       any terminal or nonterminal symbol.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       (also known as: "terminal symbols") have fall-back
**                       values which should be used if the original symbol
**                       would not parse.  This permits keywords to sometimes
**                       be used as identifiers, for example.
**    YYACTIONTYPE       is the data type used for "action codes" - numbers
**                       that indicate what to do in response to the next
**                       token.
**    ParseTOKENTYPE     is the data type used for minor type for terminal
**                       symbols.  Background: A "minor type" is a semantic
**                       value associated with a terminal or non-terminal
**                       symbols.  For example, for an "ID" terminal symbol,
**                       the minor type might be the name of the identifier.
**                       Each non-terminal can have a different minor type.
**                       Terminal symbols all have the same minor type, though.
**                       This macros defines the minor type for terminal 
**                       symbols.
**    YYMINORTYPE        is the data type used for all minor types.
**                       This is typically a union of many types, one of
**                       which is ParseTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    ParseARG_SDECL     A static variable declaration for the %extra_argument
**    ParseARG_PDECL     A parameter declaration for the %extra_argument
**    ParseARG_PARAM     Code to pass %extra_argument as a subroutine parameter
**    ParseARG_STORE     Code to store %extra_argument into yypParser
**    ParseARG_FETCH     Code to extract %extra_argument from yypParser
**    ParseCTX_*         As ParseARG_ except for %extra_context
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYNTOKEN           Number of terminal symbols
**    YY_MAX_SHIFT       Maximum value for shift actions
**    YY_MIN_SHIFTREDUCE Minimum value for shift-reduce actions
**    YY_MAX_SHIFTREDUCE Maximum value for shift-reduce actions
**    YY_ERROR_ACTION    The yy_action[] code for syntax error
**    YY_ACCEPT_ACTION   The yy_action[] code for accept
**    YY_NO_ACTION       The yy_action[] code for no-op
**    YY_MIN_REDUCE      Minimum value for reduce actions
**    YY_MAX_REDUCE      Maximum value for reduce actions
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/************* Begin control #defines *****************************************/
%%
/************* End control #defines *******************************************/
#define YY_NLOOKAHEAD ((int)(sizeof(yy_lookahead)/sizeof(yy_lookahead[0])))

namespace {

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N <= YY_MAX_SHIFT             Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   N between YY_MIN_SHIFTREDUCE       Shift to an arbitrary state then
**     and YY_MAX_SHIFTREDUCE           reduce by rule N-YY_MIN_SHIFTREDUCE.
**
**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
**   N between YY_MIN_REDUCE            Reduce by rule N-YY_MIN_REDUCE
**     and YY_MAX_REDUCE
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as either:
**
**    (A)   N = yy_action[ yy_shift_ofst[S] + X ]
**    (B)   N = yy_default[S]
**
** The (A) formula is preferred.  The B formula is used instead if
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X.
**
** The formulas above are for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
**
*********** Begin parsing tables **********************************************/
%%
/********** End of lemon-generated parsing tables *****************************/

/* The next table maps tokens (terminal symbols) into fallback tokens.  
** If a construct like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
**
** This feature can be used, for example, to cause some keywords in a language
** to revert to identifiers if they keyword does not apply in the context where
** it appears.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
%%
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
**
** After the "shift" half of a SHIFTREDUCE action, the stateno field
** actually contains the reduce action for the second half of the
** SHIFTREDUCE.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};

/* The state of the parser is completely contained in an instance of
** the following structure */

#ifndef LEMON_SUPER
#error "LEMON_SUPER must be defined."
#endif

/* outside the class so the templates above are still accessible */
void yy_destructor(YYCODETYPE yymajor, YYMINORTYPE *yypminor);
void yy_move(YYCODETYPE yymajor, YYMINORTYPE *yyDest, YYMINORTYPE *yySource);

class yypParser : public LEMON_SUPER {
  public:
    //using LEMON_SUPER::LEMON_SUPER;

    template<class ...Args>
    yypParser(Args&&... args);

    virtual ~yypParser() noexcept (true) override final;
    virtual void parse(int, ParseTOKENTYPE &&) override final;

#ifndef NDEBUG
    virtual void trace(FILE *, const char *) final override;
#endif

    virtual void reset() final override;
    virtual bool will_accept() const final override;
    virtual int fallback(int iToken) const final override;
    /*
    ** Return the peak depth of the stack for a parser.
    */
    #ifdef YYTRACKMAXSTACKDEPTH
    int yypParser::stack_peak(){
      return yyhwm;
    }
    #endif

    #ifdef YYCOVERAGE
    int coverage(FILE *out);
    #endif

    const yyStackEntry *begin() const { return yystack; }
    const yyStackEntry *end() const { return yytos + 1; }

  protected:
  private:
  yyStackEntry *yytos;          /* Pointer to top element of the stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyhwm = 0;                 /* Maximum value of yyidx */
#endif
#ifndef YYNOERRORRECOVERY
  int yyerrcnt = -1;                 /* Shifts left before out of the error */
#endif
#if YYSTACKDEPTH<=0
  int yystksz = 0;                  /* Current side of the stack */
  yyStackEntry *yystack = nullptr;        /* The parser's stack */
  yyStackEntry yystk0;          /* First stack entry */
  int yyGrowStack();
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
  yyStackEntry *yystackEnd;            /* Last entry in the stack */
#endif



  void yy_accept();
  void yy_parse_failed();
  void yy_syntax_error(int yymajor, ParseTOKENTYPE &yyminor);

  void yy_transfer(yyStackEntry *yySource, yyStackEntry *yyDest);

  void yy_pop_parser_stack();
  YYACTIONTYPE yy_find_shift_action(YYCODETYPE iLookAhead, YYACTIONTYPE stateno) const;
  YYACTIONTYPE yy_find_reduce_action(YYACTIONTYPE stateno, YYCODETYPE iLookAhead) const;

  void yy_shift(YYACTIONTYPE yyNewState, YYCODETYPE yyMajor, ParseTOKENTYPE &&yypMinor);
  YYACTIONTYPE yy_reduce(unsigned int yyruleno, int yyLookahead, const ParseTOKENTYPE &yyLookaheadToken);
  void yyStackOverflow();

#ifndef NDEBUG
  void yyTraceShift(int yyNewState, const char *zTag) const;
#else
# define yyTraceShift(X,Y)
#endif

#ifdef YYCOVERAGE
  mutable unsigned char yycoverage[YYNSTATE][YYNTOKEN] = {};
#endif

#ifndef NDEBUG
  FILE *yyTraceFILE = 0;
  const char *yyTracePrompt = 0;
#endif /* NDEBUG */

  int yyidx() const {
    return (int)(yytos - yystack);    
  }

};




#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void yypParser::trace(FILE *TraceFILE, const char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#if defined(YYCOVERAGE) || !defined(NDEBUG)
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
%%
};
#endif /* defined(YYCOVERAGE) || !defined(NDEBUG) */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
const char *const yyRuleName[] = {
%%
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.  Return the number
** of errors.  Return 0 on success.
*/
int yypParser::yyGrowStack(){
  int newSize;
  yyStackEntry *pNew;
  yyStackEntry *pOld = yystack;
  int oldSize = yystksz;

  newSize = oldSize*2 + 100;
  pNew = (yyStackEntry *)calloc(newSize, sizeof(pNew[0]));
  if( pNew ){
    yystack = pNew;
    for (int i = 0; i < oldSize; ++i) {
      pNew[i].stateno = pOld[i].stateno;
      pNew[i].major = pOld[i].major;
      yy_move(pOld[i].major, &pNew[i].minor, &pOld[i].minor);
    }
    if (pOld != &yystk0) free(pOld);
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows from %d to %d entries.\n",
              yyTracePrompt, yystksz, newSize);
    }
#endif
    yystksz = newSize;
  }
  return pNew==0; 
}
#endif


/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the 
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
void yy_destructor(
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are *not* used
    ** inside the C code.
    */
/********* Begin destructor definitions ***************************************/
%%
/********* End destructor definitions *****************************************/
    default:  break;   /* If no destructor action specified: do nothing */
  }
}


/*
 * moves an object (such as when growing the stack). 
 * Source is constructed.
 * Destination is also destructed.
 * 
 */
void yy_move(
  YYCODETYPE yymajor,     /* Type code for object to move */
  YYMINORTYPE *yyDest,     /*  */
  YYMINORTYPE *yySource     /*  */
){
  switch( yymajor ){

/********* Begin move definitions ***************************************/
%%
/********* End move definitions *****************************************/
    default:  break;   /* If no move action specified: do nothing */
      //yyDest.minor = yySource.minor;
  }
}


/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
*/
void yypParser::yy_pop_parser_stack(){
  yyStackEntry *yymsp;
  assert( yytos!=0 );
  assert( yytos > yystack );
  yymsp = yytos--;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yymsp->major]);
  }
#endif
  yy_destructor(yymsp->major, &yymsp->minor);
}


template<class ...Args>
yypParser::yypParser(Args&&... args) : LEMON_SUPER(std::forward<Args>(args)...)
{
#if YYSTACKDEPTH<=0
  if( yyGrowStack() ){
    yystack = &yystk0;
    yystksz = 1;
  }
#else
  std::memset(yystack, 0, sizeof(yystack));
#endif

  yytos = yystack;
  yystack[0].stateno = 0;
  yystack[0].major = 0;
#if YYSTACKDEPTH>0
  yystackEnd = &yystack[YYSTACKDEPTH-1];
#endif
}

void yypParser::reset() {

  while( yytos>yystack ) yy_pop_parser_stack();

#ifndef YYNOERRORRECOVERY
  yyerrcnt = -1;
#endif

  yytos = yystack;
  yystack[0].stateno = 0;
  yystack[0].major = 0;

  LEMON_SUPER::reset();
}


/* 
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/

yypParser::~yypParser() noexcept (true) {
  while( yytos>yystack ) yy_pop_parser_stack();
#if YYSTACKDEPTH<=0
  if( yystack!=&yystk0 ) free(yystack);
#endif
}


/*
** Write into out a description of every state/lookahead combination that
**
**   (1)  has not been used by the parser, and
**   (2)  is not a syntax error.
**
** Return the number of missed state/lookahead combinations.
*/
#if defined(YYCOVERAGE)
int yypParser::coverage(FILE *out){
  int stateno, iLookAhead, i;
  int nMissed = 0;
  for(stateno=0; stateno<YYNSTATE; stateno++){
    i = yy_shift_ofst[stateno];
    for(iLookAhead=0; iLookAhead<YYNTOKEN; iLookAhead++){
      if( yy_lookahead[i+iLookAhead]!=iLookAhead ) continue;
      if( yycoverage[stateno][iLookAhead]==0 ) nMissed++;
      if( out ){
        fprintf(out,"State %d lookahead %s %s\n", stateno,
                yyTokenName[iLookAhead],
                yycoverage[stateno][iLookAhead] ? "ok" : "missed");
      }
    }
  }
  return nMissed;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
YYACTIONTYPE yypParser::yy_find_shift_action(
  YYCODETYPE iLookAhead,    /* The look-ahead token */
  YYACTIONTYPE stateno      /* Current state number */
) const {
  int i;

  if( stateno>YY_MAX_SHIFT ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
#if defined(YYCOVERAGE)
  yycoverage[stateno][iLookAhead] = 1;
#endif
  do{
    i = yy_shift_ofst[stateno];
    assert( i>=0 );
    /* assert( i+YYNTOKEN<=(int)YY_NLOOKAHEAD ); */
    assert( iLookAhead!=YYNOCODE );
    assert( iLookAhead < YYNTOKEN );
    i += iLookAhead;
    if( i>=YY_NLOOKAHEAD || yy_lookahead[i]!=iLookAhead ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        assert( yyFallback[iFallback]==0 ); /* Fallback loop must terminate */
        iLookAhead = iFallback;
        continue;
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( 
#if YY_SHIFT_MIN+YYWILDCARD<0
          j>=0 &&
#endif
#if YY_SHIFT_MAX+YYWILDCARD>=YY_ACTTAB_COUNT
          j<YY_ACTTAB_COUNT &&
#endif
          j<(int)(sizeof(yy_lookahead)/sizeof(yy_lookahead[0])) &&
          yy_lookahead[j]==YYWILDCARD && iLookAhead>0
        ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead],
               yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
      return yy_default[stateno];
    }else{
      return yy_action[i];
    }
  }while(1);
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
*/
YYACTIONTYPE yypParser::yy_find_reduce_action(
  YYACTIONTYPE stateno,     /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
) const {
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
void yypParser::yyStackOverflow(){
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yytos>yystack ) yy_pop_parser_stack();
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
/******** Begin %stack_overflow code ******************************************/
%%
/******** End %stack_overflow code ********************************************/
  LEMON_SUPER::stack_overflow();
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
void yypParser::yyTraceShift(int yyNewState, const char *zTag) const {
  if( yyTraceFILE ){
    if( yyNewState<YYNSTATE ){
      fprintf(yyTraceFILE,"%s%s '%s', go to state %d\n",
         yyTracePrompt, zTag, yyTokenName[yytos->major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%s%s '%s', pending reduce %d\n",
         yyTracePrompt, zTag, yyTokenName[yytos->major],
         yyNewState - YY_MIN_REDUCE);
    }
  }
}
#endif

/*
** Perform a shift action.
*/
void yypParser::yy_shift(
  YYACTIONTYPE yyNewState,               /* The new state to shift in */
  YYCODETYPE yyMajor,                  /* The major token to shift in */
  ParseTOKENTYPE &&yyMinor      /* The minor token to shift in */
){
  yytos++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yyidx()>yyhwm ){
    yyhwm++;
    assert(yyhwm == yyidx());
  }
#endif
#if YYSTACKDEPTH>0 
  if( yytos>yystackEnd ){
    yytos--;
    yyStackOverflow();
    return;
  }
#else
  if( yytos>=&yystack[yystksz] ){
    if( yyGrowStack() ){
      yytos--;
      yyStackOverflow();
      return;
    }
  }
#endif
  if( yyNewState > YY_MAX_SHIFT ){
    yyNewState += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
  }
  yytos->stateno = yyNewState;
  yytos->major = yyMajor;
  //yytos->minor.yy0 = yyMinor;
  //yy_move also calls the destructor...
  //yy_move<ParseTOKENTYPE>(std::addressof(yytos->minor.yy0), std::addressof(yyMinor));
  yy_constructor<ParseTOKENTYPE>(std::addressof(yytos->minor.yy0), std::move(yyMinor));
  yyTraceShift(yyNewState, "Shift");
}

/* For rule J, yyRuleInfoLhs[J] contains the symbol on the left-hand side
** of that rule */
static const YYCODETYPE yyRuleInfoLhs[] = {
%%
};

/* For rule J, yyRuleInfoNRhs[J] contains the negative of the number
** of symbols on the right-hand side of that rule. */
static const signed char yyRuleInfoNRhs[] = {
%%
};

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
YYACTIONTYPE yypParser::yy_reduce(
  unsigned int yyruleno,       /* Number of the rule by which to reduce */
  int yyLookahead,             /* Lookahead token, or YYNOCODE if none */
  const ParseTOKENTYPE &yyLookaheadToken  /* Value of the lookahead token */
){
  int yygoto;                     /* The next state */
  YYACTIONTYPE yyact;                      /* The next action */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  yymsp = yytos;
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    yysize = yyRuleInfoNRhs[yyruleno];
    if( yysize ){
      fprintf(yyTraceFILE, "%sReduce %d [%s], go to state %d.\n",
        yyTracePrompt,
        yyruleno, yyRuleName[yyruleno], yymsp[yysize].stateno);
    }else{
      fprintf(yyTraceFILE, "%sReduce %d [%s].\n",
        yyTracePrompt, yyruleno, yyRuleName[yyruleno]);
    }
  }
#endif /* NDEBUG */

  /* Check that the stack is large enough to grow by a single entry
  ** if the RHS of the rule is empty.  This ensures that there is room
  ** enough on the stack to push the LHS value */
  if( yyRuleInfoNRhs[yyruleno]==0 ){
#ifdef YYTRACKMAXSTACKDEPTH
    if( yyidx()>yyhwm ){
      yyhwm++;
      assert(yyhwm == yyidx());
    }
#endif
#if YYSTACKDEPTH>0 
    if( yytos>=yystackEnd ){
      yyStackOverflow();
      /* The call to yyStackOverflow() above pops the stack until it is
      ** empty, causing the main parser loop to exit.  So the return value
      ** is never used and does not matter. */
      return 0;
    }
#else
    if( yytos>=&yystack[yystksz-1] ){
      if( yyGrowStack() ){
        yyStackOverflow();
        /* The call to yyStackOverflow() above pops the stack until it is
        ** empty, causing the main parser loop to exit.  So the return value
        ** is never used and does not matter. */
        return 0;
      }
      yymsp = yytos;
    }
#endif
  }

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
/********** Begin reduce actions **********************************************/
%%
/********** End reduce actions ************************************************/
  };
  assert( yyruleno<sizeof(yyRuleInfoLhs)/sizeof(yyRuleInfoLhs[0]) );
  yygoto = yyRuleInfoLhs[yyruleno];
  yysize = yyRuleInfoNRhs[yyruleno];
  yyact = yy_find_reduce_action(yymsp[yysize].stateno,(YYCODETYPE)yygoto);

  /* There are no SHIFTREDUCE actions on nonterminals because the table
  ** generator has simplified them to pure REDUCE actions. */
  assert( !(yyact>YY_MAX_SHIFT && yyact<=YY_MAX_SHIFTREDUCE) );

  /* It is not possible for a REDUCE to be followed by an error */
  assert( yyact!=YY_ERROR_ACTION );

  yymsp += yysize+1;
  yytos = yymsp;
  yymsp->stateno = (YYACTIONTYPE)yyact;
  yymsp->major = (YYCODETYPE)yygoto;
  yyTraceShift(yyact, "... then shift");
  return yyact;
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
void yypParser::yy_parse_failed(){
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yytos>yystack ) yy_pop_parser_stack();
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
%%
/************ End %parse_failure code *****************************************/
  LEMON_SUPER::parse_failure();
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
void yypParser::yy_syntax_error(
  int yymajor,                   /* The major type of the error token */
  ParseTOKENTYPE &yyminor        /* The minor type of the error token */
){
//#define TOKEN yyminor
  auto &TOKEN = yyminor;
/************ Begin %syntax_error code ****************************************/
%%
/************ End %syntax_error code ******************************************/
  LEMON_SUPER::syntax_error(yymajor, TOKEN);
}

/*
** The following is executed when the parser accepts
*/
void yypParser::yy_accept(){
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
#ifndef YYNOERRORRECOVERY
  yyerrcnt = -1;
#endif
  assert( yytos==yystack );
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/
%%
/*********** End %parse_accept code *******************************************/
  LEMON_SUPER::parse_accept();
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "ParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/



void yypParser::parse(
  int yymajor,                 /* The major token code number */
  ParseTOKENTYPE &&yyminor       /* The value for the token */
){
  //YYMINORTYPE yyminorunion;
  YYACTIONTYPE yyact;            /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif

  assert( yytos!=0 );
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif

  yyact = yytos->stateno;
#ifndef NDEBUG
  if( yyTraceFILE ){
    if( yyact < YY_MIN_REDUCE ){
      fprintf(yyTraceFILE,"%sInput '%s' in state %d\n",
              yyTracePrompt,yyTokenName[yymajor],yyact);
    }else{
      fprintf(yyTraceFILE,"%sInput '%s' with pending reduce %d\n",
              yyTracePrompt,yyTokenName[yymajor],yyact-YY_MIN_REDUCE);
    }
  }
#endif

  do{
    assert( yyact==yytos->stateno );
    yyact = yy_find_shift_action((YYCODETYPE)yymajor,yyact);
    if( yyact >= YY_MIN_REDUCE ){
      yyact = yy_reduce(yyact-YY_MIN_REDUCE,yymajor, yyminor);
    }else if( yyact <= YY_MAX_SHIFTREDUCE ){
      yy_shift(yyact,(YYCODETYPE)yymajor,std::move(yyminor));
#ifndef YYNOERRORRECOVERY
      yyerrcnt--;
#endif
      break;
    }else if( yyact==YY_ACCEPT_ACTION ){
      yytos--;
      yy_accept();
      return;
    }else{
      assert( yyact == YY_ERROR_ACTION );
      //yyminorunion.yy0 = yyminor;
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yyerrcnt<0 ){
        yy_syntax_error(yymajor,yyminor);
      }
      yymx = yytos->major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        //yy_destructor(yyminor);
        yymajor = YYNOCODE;
      }else{
        while( yytos >= yystack
            && (yyact = yy_find_reduce_action(
                        yytos->stateno,
                        YYERRORSYMBOL)) > YY_MAX_SHIFTREDUCE
        ){
          yy_pop_parser_stack();
        }
        if( yytos < yystack || yymajor==0 ){
          //yy_destructor(yyminor);
          yy_parse_failed();
#ifndef YYNOERRORRECOVERY
          yyerrcnt = -1;
#endif
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          yy_shift(yyact,YYERRORSYMBOL,std::move(yyminor));
        }
      }
      yyerrcnt = 3;
      yyerrorhit = 1;
      if( yymajor==YYNOCODE ) break;
      yyact = yytos->stateno;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yymajor,yyminor);
      //yy_destructor(yyminor);
      break;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yyerrcnt<=0 ){
        yy_syntax_error(yymajor,yyminor);
      }
      yyerrcnt = 3;
      //yy_destructor(yyminor);
      if( yyendofinput ){
        yy_parse_failed();
#ifndef YYNOERRORRECOVERY
        yyerrcnt = -1;
#endif
      }
      break;
#endif
    }
  }while( yytos>yystack );
#ifndef NDEBUG
  if( yyTraceFILE ){
    yyStackEntry *i;
    char cDiv = '[';
    fprintf(yyTraceFILE,"%sReturn. Stack=",yyTracePrompt);
    for(i=&yystack[1]; i<=yytos; i++){
      fprintf(yyTraceFILE,"%c%s", cDiv, yyTokenName[i->major]);
      cDiv = ' ';
    }
    if (cDiv == '[') fprintf(yyTraceFILE,"[");
    fprintf(yyTraceFILE,"]\n");
  }
#endif
  return;
}


bool yypParser::will_accept() const {


  struct stack_entry {
    YYACTIONTYPE stateno;
    YYCODETYPE major;
  };

  YYACTIONTYPE yyact;
  const YYCODETYPE yymajor = 0;
  ParseTOKENTYPE yyminor;
  std::vector<stack_entry> stack;


  // copy stack to stack.
  stack.reserve(yyidx()+1);
  std::transform(begin(), end(), std::back_inserter(stack), [](const yyStackEntry &e){
    return stack_entry({e.stateno, e.major});
  });

  do {
    yyact = yy_find_shift_action(yymajor, stack.back().stateno);


    if( yyact==YY_ACCEPT_ACTION ) return true;
    if( yyact >= YY_MIN_REDUCE ){
      // reduce...
      unsigned yyruleno = yyact - YY_MIN_REDUCE;

      YYCODETYPE yygoto = yyRuleInfoLhs[yyruleno];
      int yysize = -yyRuleInfoNRhs[yyruleno]; /* stored as negative value */

      while (yysize--) stack.pop_back();

      yyact = yy_find_reduce_action(stack.back().stateno, yygoto);
      stack.push_back({yyact, yygoto});
    }
    else {
      return false;
    }

  } while (!stack.empty());

  return false;


}

/*
** Return the fallback token corresponding to canonical token iToken, or
** 0 if iToken has no fallback.
*/
int yypParser::fallback(int iToken) const {
#ifdef YYFALLBACK
  if( iToken<(int)(sizeof(yyFallback)/sizeof(yyFallback[0])) ){
    return yyFallback[iToken];
  }
#else
  (void)iToken;
#endif
  return 0;
}



} // namespace
