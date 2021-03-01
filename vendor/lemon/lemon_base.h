#ifndef __lemon_base_h__
#define __lemon_base_h__
#include <cstdio>

template <class TokenType> class lemon_base
{
  public:
    typedef TokenType token_type;

    virtual ~lemon_base () = default;

    virtual void parse (int yymajor, TokenType && yyminor) = 0;

    virtual void trace (FILE *, const char *) {}

    virtual bool will_accept () const = 0;
    virtual int fallback (int iToken) const = 0;

    virtual void reset () {}

  protected:
    virtual void parse_accept () {}
    virtual void parse_failure () {}
    virtual void stack_overflow () {}
    virtual void syntax_error (int yymajor, TokenType & yyminor) {}
    lemon_base () {}

  private:
    lemon_base (const lemon_base &) = delete;
    lemon_base (lemon_base &&) = delete;
    lemon_base & operator= (const lemon_base &) = delete;
    lemon_base & operator= (lemon_base &&) = delete;
};

#endif