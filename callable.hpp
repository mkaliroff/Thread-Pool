#ifndef CALLABLE_HPP
#define CALLABLE_HPP

#include<memory> // shared_ptr


class Callable
{
public:
virtual ~Callable() = default;

virtual void operator()() = 0;
};

class CallableFunction: public Callable
{
public:
    template <typename Args>
    explicit CallableFunction(void (*function)(Args), Args argument);

    template <typename Object, typename Args>
    explicit CallableFunction(void (Object::*function)(Args), Object &object, Args argument);

	inline void operator()()
	{
    	m_function->Invoke();
	}

private:
	class Ifunction
    {
	public:
		virtual ~Ifunction() = default;
		virtual void Invoke() = 0;		
	};

    template <typename Args>
	class Ifunction_Free:  public Ifunction
    {
	public:
		Ifunction_Free(void (*function)(Args), Args& args);
		~Ifunction_Free() override = default;
		void Invoke() override;

	private:
		void (*m_function)(Args);
	    Args m_argument;
	};

    template <typename Object, typename Args>
	class Ifunction_Object: public Ifunction
    {
	public:
		Ifunction_Object(void (Object::*function)(Args), Object& object, Args& args);

		~Ifunction_Object() override = default;

		void Invoke() override;

	private:
		void (Object::*m_function)(Args);
		Object &m_object;
        Args m_argument;
	};

	std::shared_ptr<Ifunction> m_function;
};

template <typename Args>
CallableFunction::CallableFunction(void (*function)(Args), Args argument): 
m_function(new Ifunction_Free<Args>(function, argument)) 
{
	//empty
}

template <typename Object, typename Args>
CallableFunction::CallableFunction(void (Object::*function)(Args), Object &object, Args argument)
: m_function(new Ifunction_Object<Object,Args>(function, object ,argument))
{
	//empty
}


template <typename Args>
CallableFunction::Ifunction_Free<Args>::Ifunction_Free(void (*function)(Args), Args& args)
:m_function(function), m_argument(args)
{
	//empty
}
template <typename Args>
void CallableFunction::Ifunction_Free<Args>::Invoke() 
{
	m_function(m_argument);
}

template <typename Object, typename Args>
CallableFunction::Ifunction_Object<Object,Args>::Ifunction_Object(void
 (Object::*function)(Args), Object& object, Args& args) : m_function(function), 
 m_object(object),m_argument(args)
{
	//empty
}
template <typename Object, typename Args>
void CallableFunction::Ifunction_Object<Object,Args>::Invoke() 
{
	(m_object.*m_function)(m_argument);
}

#endif /* CALLABLE_HPP*/