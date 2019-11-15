/*
 * Copyright (c) 2014 Eran Pe'er.
 *
 * This program is made available under the terms of the MIT License.
 *
 * Created on Mar 10, 2014
 */
#pragma once

#include "fakeit/DomainObjects.hpp"
#include "fakeit/MockImpl.hpp"
#include "fakeit/ActualInvocation.hpp"
#include "fakeit/Prototype.hpp"

namespace fakeit {
    namespace internal {
    }
    using namespace fakeit::internal;

	// Function type manipulation utilities.
	struct func_traits{

	#ifdef _MSC_VER
		// On MSC, always define the __cdecl variants. Only they are needed in x64.
		#define CC_CDECL __cdecl
	#else
		// On non-MSC, don't worry about it.
		#define CC_CDECL
	#endif
	
		// These are the default implementations whose presence always makes sense.

		template<typename T, typename R, typename... arglist>
		static R(CC_CDECL T::* remove_cv( R(CC_CDECL T::*vMethod )( arglist... ) const volatile ))( arglist... ) {
			return reinterpret_cast< R ( T::* )( arglist... ) >( vMethod );
		};
	
		template<typename T, typename R, typename... arglist>
		static R(CC_CDECL T::* remove_cv( R(CC_CDECL T::*vMethod )( arglist... ) volatile ))( arglist... ) {
			return reinterpret_cast< R ( T::* )( arglist... ) >( vMethod );
		};

		template<typename T, typename R, typename... arglist>
		static R(CC_CDECL T::* remove_cv( R(CC_CDECL T::*vMethod )( arglist... ) const ))( arglist... ) {
			return reinterpret_cast< R ( T::* )( arglist... ) >( vMethod );
		};

		template<typename T, typename R, typename... arglist>
		static R(CC_CDECL T::* remove_cv( R(CC_CDECL T::*vMethod )( arglist... ) ))( arglist... ) {
			return vMethod;
		};

	// On 32-bit msc, define also the other calling conventions.
	#if defined( _MSC_VER ) && ! defined( _WIN64 )

		// __stdcall, used in normal functions and COM interfaces.

		template<typename T, typename R, typename... arglist>
		static R(__stdcall T::* remove_cv( R(__stdcall T::*vMethod )( arglist... ) const volatile ))( arglist... ) {
			return reinterpret_cast< R ( T::* )( arglist... ) >( vMethod );
		};
	
		template<typename T, typename R, typename... arglist>
		static R(__stdcall T::* remove_cv( R(__stdcall T::*vMethod )( arglist... ) volatile ))( arglist... ) {
			return reinterpret_cast< R( __stdcall T::* )( arglist... ) >( vMethod );
		};

		template<typename T, typename R, typename... arglist>
		static R(__stdcall T::* remove_cv( R(__stdcall T::*vMethod )( arglist... ) const ))( arglist... ) {
			return reinterpret_cast< R( __stdcall T::* )( arglist... ) >( vMethod );
		};

		template<typename T, typename R, typename... arglist>
		static R(__stdcall T::* remove_cv( R(__stdcall T::*vMethod )( arglist... ) ))( arglist... ) {
			return vMethod;
		};

		// __thiscall, used in member functions.
	
		template<typename T, typename R, typename... arglist>
		static R(__stdcall T::* remove_cv( R(__stdcall T::*vMethod )( arglist... ) const volatile ))( arglist... ) {
			return reinterpret_cast< R ( T::* )( arglist... ) >( vMethod );
		};
	
		template<typename T, typename R, typename... arglist>
		static R(__stdcall T::* remove_cv( R(__stdcall T::*vMethod )( arglist... ) volatile ))( arglist... ) {
			return reinterpret_cast< R( __stdcall T::* )( arglist... ) >( vMethod );
		};

		template<typename T, typename R, typename... arglist>
		static R(__stdcall T::* remove_cv( R(__stdcall T::*vMethod )( arglist... ) const ))( arglist... ) {
			return reinterpret_cast< R( __stdcall T::* )( arglist... ) >( vMethod );
		};

		template<typename T, typename R, typename... arglist>
		static R(__stdcall T::* remove_cv( R(__stdcall T::*vMethod )( arglist... ) ))( arglist... ) {
			return vMethod;
		};
	#endif

		template<typename Func>
		using remove_cv_t = decltype( remove_cv( std::declval<Func>() ) );
	};

    template<typename C, typename ... baseclasses>
    class Mock : public ActualInvocationsSource {
        MockImpl<C, baseclasses...> impl;
    public:
        virtual ~Mock() = default;

        static_assert(std::is_polymorphic<C>::value, "Can only mock a polymorphic type");

        Mock() : impl(Fakeit) {
        }

        explicit Mock(C &obj) : impl(Fakeit, obj) {
        }

        virtual C &get() {
            return impl.get();
        }

//		std::shared_ptr<C> getShared() {
//			return impl.getShared();
//		}
        
		C &operator()() {
            return get();
        }

        void Reset() {
            impl.reset();
        }

		void ClearInvocationHistory() {
			impl.clear();
		}

		template<class DATA_TYPE, typename ... arglist,
                class = typename std::enable_if<std::is_member_object_pointer<DATA_TYPE C::*>::value>::type>
        DataMemberStubbingRoot<C, DATA_TYPE> Stub(DATA_TYPE C::* member, const arglist &... ctorargs) {
            return impl.stubDataMember(member, ctorargs...);
        }

        template<int id, typename R, typename T, typename... arglist, class = typename std::enable_if<
                !std::is_void<R>::value && std::is_base_of<T, C>::value>::type>
        MockingContext<R, arglist...> stubImpl(R(T::*vMethod)(arglist...)) {
            return impl.template stubMethod<id>(vMethod);
        }

        template<int id, typename R, typename T, typename... arglist, class = typename std::enable_if<
                std::is_void<R>::value && std::is_base_of<T, C>::value>::type>
        MockingContext<void, arglist...> stubImpl(R(T::*vMethod)(arglist...)) {
            auto methodWithoutConstVolatile = reinterpret_cast<void (T::*)(arglist...)>(vMethod);
            return impl.template stubMethod<id>(methodWithoutConstVolatile);
        }

		template<int id, typename Func>
		auto stub( Func func ) -> decltype( stubImpl<id>( func_traits::remove_cv( func ) ) )
		{
			return stubImpl<id>( func_traits::remove_cv( func ) );
		}

		//template<int it, typename Method >
//		auto stub( Method func ) -> decltype(  )

        DtorMockingContext dtor() {
            return impl.stubDtor();
        }

        void getActualInvocations(std::unordered_set<Invocation *> &into) const override {
            impl.getActualInvocations(into);
        }

    };
}