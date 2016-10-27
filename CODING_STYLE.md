### Please use the following code style/guidelines

- use QT wherever it's possible (except there is a good reason)
- use unix line endings (not windows)
- indent your code with TABs instead of spaces
- your files should end with a newline
- names are camel case
- use utf8 file encoding (ANSI encoding is strictly forbidden!)
- use speaking names for variables.
- avoid code dups -> if you write similar code blocks more the 2 times -> refactoring!
- avoid compiler macros (#ifdef #define ...) where possible
- class member variables must prefixed with underscore `int _myMemberVar`
- follow this rule for curly brackets

```
bad:
if (conditon) {
	code
}

good:
if (condition)
{
	code
}
```
- initializer list on constructors:

```
bad:
MyClass::MyClass()
	: myVarA(0), myVarB("eee"), myVarC(true)
{
}

MyClass::MyClass() : myVarA(0),
	myVarB("eee"),
	myVarC(true)
{
}

good:
MyClass::MyClass()
	: myVarA(0)
	, myVarB("eee")
	, myVarC(true)
{
}
```

- pointer declaration

```
bad:
int *foo;
int * fooFoo;

good:
int* foo;
```

