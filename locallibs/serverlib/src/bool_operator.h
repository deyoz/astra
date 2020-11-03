#ifndef SERVERLIB_BOOL_OPERATOR_H
#define SERVERLIB_BOOL_OPERATOR_H

#define BOOL_OPERATOR_S struct bool_operator_conv {int i;}; \
typedef int (bool_operator_conv::*bool_operator_conv_ptr);

#define BOOL_OPERATOR_DECL BOOL_OPERATOR_S; operator bool_operator_conv_ptr() const;

#define BOOL_OPERATOR_DEF(name, condition)  name::operator name::bool_operator_conv_ptr() const\
{ if(condition) return &bool_operator_conv::i ; return 0;}

#define BOOL_OPERATOR(condition) BOOL_OPERATOR_S ; operator bool_operator_conv_ptr() const \
{ if(condition) return &bool_operator_conv::i ; return 0;}


#if 0 
//example
class A{
int p;
A(int pp):p(pp){}
BOOL_OPERATOR(p!=0)

};
class B{
int p;
B(int pp):p(pp){}
BOOL_OPERATOR_DECL;
};
BOOL_OPERATOR_DEF(B,p!=0);
#endif


#endif /* SERVERLIB_BOOL_OPERATOR_H */

