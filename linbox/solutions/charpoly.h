/* linbox/solutions/charpoly.h
 * Copyright (C) 2005 Clement Pernet
 *
 * Written by Clement Pernet <clement.pernet@imag.fr>
 *
 * ========LICENCE========
 * This file is part of the library LinBox.
 *
  * LinBox is free software: you can redistribute it and/or modify
 * it under the terms of the  GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * ========LICENCE========
 */

#ifndef __LINBOX_charpoly_H
#define __LINBOX_charpoly_H

#include "linbox/solutions/methods.h"
#include "linbox/util/debug.h"
#include "linbox/field/field-traits.h"
#include "linbox/matrix/dense-matrix.h"
#include "linbox/matrix/matrix-domain.h"
#include "linbox/randiter/random-prime.h"
#include "linbox/algorithms/bbcharpoly.h"
// Namespace in which all LinBox library code resides

namespace LinBox
{

	// for specialization with respect to the DomainCategory
	template< class Blackbox, class Polynomial, class MyMethod, class DomainCategory>
	Polynomial& charpoly ( Polynomial            &P,
                               const Blackbox        &A,
                               const DomainCategory  &tag,
                               const MyMethod        &M);


	/*	//error handler for rational domain
		template <class Blackbox, class Polynomial>
		Polynomial &charpoly (Polynomial& P,
		const Blackbox& A,
		const RingCategories::RationalTag& tag,
		const Method::Auto& M)
		{
		throw LinboxError("LinBox ERROR: charpoly is not yet defined over a rational domain");
		}
		*/
	/** \brief  ...using an optional Method parameter
	  \param P - the output characteristic polynomial.  If the polynomial
	  is of degree d, this random access container has size d+1, the 0-th
	  entry is the constant coefficient and the d-th is 1 since the charpoly
	  is monic.
	  \param A - a blackbox matrix
	  Optional \param M - the method object.  Generally, the default
	  object suffices and the algorithm used is determined by the class of M.
	  Basic methods are Method::Blackbox, Method::Elimination, and
	  Method::Auto (the default).
	  See methods.h for more options.
	  \return a reference to P.
	  */
	template <class Blackbox, class Polynomial, class MyMethod>
	Polynomial& charpoly (Polynomial         & P,
                              const Blackbox     & A,
                              const MyMethod     & M){
		return charpoly ( P, A, typename FieldTraits<typename Blackbox::Field>::categoryTag(), M);
	}


	/// \brief ...using default method
	template<class Blackbox, class Polynomial>
	Polynomial& charpoly (Polynomial        & P,
                              const Blackbox    & A)
	{
		return charpoly (P, A, Method::Auto());
	}

	// The charpoly with Auto Method
	template <class Blackbox, class Polynomial>
	Polynomial& charpoly (Polynomial                       & P,
                              const Blackbox                   & A,
                              const RingCategories::ModularTag & tag,
                              const Method::Auto             & M)
	{
		//return charpoly(P, A, tag, Method::Blackbox(M));
		return charpoly(P, A, tag, Method::DenseElimination(M));
	}

	// The charpoly with Auto Method
	template <class Domain, class Polynomial>
	Polynomial& charpoly (Polynomial                       & P,
			      const SparseMatrix<Domain>       & A,
			      const RingCategories::ModularTag & tag,
			      const Method::Auto             & M)
	{
                // bb method broken, default to dense method
		return charpoly(P, A, tag, Method::DenseElimination(M));
//		return charpoly(P, A, tag, Method::Blackbox(M));
	}

	// The charpoly with Auto Method
	template<class Domain, class Polynomial>
	Polynomial& charpoly (Polynomial                       & P,
			      const BlasMatrix<Domain>         & A,
			      const RingCategories::ModularTag & tag,
			      const Method::Auto             & M)
	{
		return charpoly(P, A, tag, Method::DenseElimination(M));
	}

	// The charpoly with Elimination Method
	template<class Blackbox, class Polynomial>
        Polynomial& charpoly (Polynomial                       & P,
                              const Blackbox                   & A,
                              const RingCategories::ModularTag & tag,
                              const Method::Elimination        & M)
	{
		return charpoly(P, A, tag, Method::DenseElimination(M));
	}


	/** @brief Compute the characteristic polynomial over \f$\mathbf{Z}_p\f$.
	 *
	 * Compute the characteristic polynomial of a matrix using dense
	 * elimination methods

	 * @param P Polynomial where to store the result
	 * @param A Blackbox representing the matrix
	 * @param tag
	 * @param M
	 */
	template <class Blackbox, class Polynomial >
    Polynomial& charpoly (Polynomial                       & P,
                          const Blackbox                   & A,
                          const RingCategories::ModularTag & tag,
                          const Method::DenseElimination    & M)
	{
		if (A.coldim() != A.rowdim())
			throw LinboxError("LinBox ERROR: matrix must be square for characteristic polynomial computation\n");

		BlasMatrix< typename Blackbox::Field >     BBB (A);
		BlasMatrixDomain< typename Blackbox::Field > BMD (BBB.field());
                    //BlasVector<typename Blackbox::Field,Polynomial> P2(A.field(),P);
		BMD.charpoly (P, BBB);
		return P;//= P2.getRep() ;
	}


}

#include "linbox/algorithms/matrix-hom.h"

#include "linbox/algorithms/rational-cra2.h"
#include "linbox/algorithms/varprec-cra-builder-early-multip.h"
#include "linbox/algorithms/charpoly-rational.h"

namespace LinBox
{
	template <class Blackbox, class MyMethod>
	struct IntegerModularCharpoly {
		const Blackbox &A;
		const MyMethod &M;

		IntegerModularCharpoly(const Blackbox& b, const MyMethod& n) :
			A(b), M(n)
		{}

		template<typename Field, class Polynomial>
		IterationResult operator()(Polynomial& P, const Field& F) const
		{
			typedef typename Blackbox::template rebind<Field>::other FBlackbox;
			FBlackbox Ap(A, F);
			charpoly (P, Ap, typename FieldTraits<Field>::categoryTag(), M);
			return IterationResult::CONTINUE;
			// std::cerr << "Charpoly(A) mod "<<F.characteristic()<<" = "<<P;
			// integer p;
			// F.characteristic(p);
			// std::cerr<<"Charpoly(A) mod "<<p<<" = "<<P;
		}
	};

	template <class Blackbox, class Polynomial>
	Polynomial& charpoly (Polynomial                       & P,
						  const Blackbox                   & A,
						  const RingCategories::IntegerTag & tag,
						  const Method::Auto	       & M)
	{
		commentator().start ("Integer Charpoly", "Icharpoly");
		// bb method broken, default to dense method
		if (1/* (A.rowdim() < 1000) && (A.coldim() <1000) */)
			charpoly(P, A, tag, Method::DenseElimination(M) );
		else
			charpoly(P, A, tag, Method::Blackbox(M) );
		commentator().stop ("done", NULL, "Icharpoly");
		return P;
	}

}

#if 0 // CIA is buggy (try with matrix(ZZ,4,[1..16]) )
//#if defined(__LINBOX_HAVE_NTL)

#include "linbox/algorithms/cia.h"
namespace LinBox
{
#if 0

	// The charpoly with Auto Method
	template<class Blackbox, class Polynomial>
	Polynomial& charpoly (Polynomial                        &P,
			      const Blackbox                    &A,
			      const RingCategories::IntegerTag  &tag,
			      const Method::Auto              &M)
	{
                    // bb method broken, default to dense method
                return charpoly(P, A, tag, Method::DenseElimination(M));
//		return charpoly(P, A, tag, Method::Blackbox(M));
	}
#endif

	template < class IntRing, class Polynomial>
                   Polynomial& charpoly (Polynomial                       & P,
			      const BlasMatrix<IntRing>         & A,
			      const RingCategories::IntegerTag & tag,
			      const Method::Auto             & M)
	{
		commentator().start ("BlasMatrix Integer Charpoly", "Icharpoly");
		charpoly(P, A, tag, Method::DenseElimination(M) );
		commentator().stop ("done", NULL, "Icharpoly");
		return P;
	}


#if 0
	template < class IntRing, class Polynomial >
	Polynomial& charpoly (Polynomial                       & P,
			      const BlasMatrix<IntRing>         & A,
			      const RingCategories::IntegerTag & tag,
			      const Method::Auto             & M)
	{
		commentator().start ("BlasMatrix Integer Charpoly", "Icharpoly");
		charpoly(P, A, tag, Method::DenseElimination(M) );
		commentator().stop ("done", NULL, "Icharpoly");
		return P;
	}
#endif

	/** @brief Compute the characteristic polynomial over {\bf Z}
	 *
	 * Compute the characteristic polynomial of a matrix using dense
	 * elimination methods

	 * @param P Polynomial where to store the result
	 * @param A \ref Black-Box representing the matrix
	 */


	template <class Blackbox, class Polynomial>
	Polynomial& charpoly (Polynomial                       & P,
			      const Blackbox                   & A,
			      const RingCategories::IntegerTag & tag,
			      const Method::DenseElimination    & M)
	{
		if (A.coldim() != A.rowdim())
			throw LinboxError("LinBox ERROR: matrix must be square for characteristic polynomial computation\n");
		typename Givaro::Poly1Dom<typename Blackbox::Field, Givaro::Dense>::Element Pg;
                std::cerr<<"CIA"<<std::endl;
		return P = cia (Pg, A, M);
	}

	/** Compute the characteristic polynomial over {\bf Z}
	 *
	 * Compute the characteristic polynomial of a matrix, represented via
	 * a blackBox.
	 *
	 * @param P Polynomial where to store the result
	 * @param A \ref Black-Box representing the matrix
	 */
	template <class Blackbox, class Polynomial/*, class Categorytag*/ >
	Polynomial& charpoly (Polynomial                       & P,
			      const Blackbox                   & A,
			      const RingCategories::IntegerTag & tag,
			      const Method::Blackbox           & M)
	{
		if (A.coldim() != A.rowdim())
			throw LinboxError("LinBox ERROR: matrix must be square for characteristic polynomial computation\n");
// 		typename Givaro::Poly1Dom<typename Blackbox::Field>::Element Pg;
// 		return P = BBcharpoly::blackboxcharpoly (Pg, A, tag, M);
                    // BB method broken: default to dense method
                return charpoly(P, A, tag, Method::DenseElimination(M) );
                    //return BBcharpoly::blackboxcharpoly (P, A, tag, M);
	}

}

#else //  no NTL

#include "linbox/ring/modular.h"
#include "linbox/algorithms/cra-domain.h"
#include "linbox/algorithms/cra-builder-full-multip.h"
#include "linbox/algorithms/cra-builder-early-multip.h"
#include "linbox/algorithms/matrix-hom.h"

namespace LinBox
{

#if 0
#include "linbox/algorithms/rational-cra2.h"
#include "linbox/algorithms/varprec-cra-builder-early-multip.h"
#include "linbox/algorithms/charpoly-rational.h"

	namespace LinBox
{
	template <class Blackbox, class MyMethod>
	struct IntegerModularCharpoly {
		const Blackbox &A;
		const MyMethod &M;

		IntegerModularCharpoly(const Blackbox& b, const MyMethod& n) :
			A(b), M(n)
		{}

		template<typename Polynomial, typename Field>
		IterationResult operator()(Polynomial& P, const Field& F) const {
			typedef typename Blackbox::template rebind<Field>::other FBlackbox;
			FBlackbox * Ap;
			MatrixHom::map(Ap, A, F);
			charpoly( P, *Ap, typename FieldTraits<Field>::categoryTag(), M);
			integer p;
			F.characteristic(p);
                            //std::cerr<<"Charpoly(A) mod "<<p<<" = "<<P;

			delete Ap;
			return IterationResult::CONTINUE;
		}
	};
#endif

	template <class Blackbox, class Polynomial>
	Polynomial& charpoly (Polynomial                       & P,
			      const Blackbox                   & A,
			      const RingCategories::IntegerTag & tag,
			      const Method::Blackbox           & M)
	{
		if (A.coldim() != A.rowdim())
			throw LinboxError("LinBox ERROR: matrix must be square for characteristic polynomial computation\n");

		commentator().start ("Integer BlackBox Charpoly : No NTL installation -> chinese remaindering", "IbbCharpoly");

        typedef Givaro::ModularBalanced<double> Field;
		PrimeIterator<IteratorCategories::HeuristicTag> genprime(FieldTraits<Field>::bestBitSize(A.coldim()));
#if 0
		typename Blackbox::ConstIterator it = A.Begin();
		typename Blackbox::ConstIterator it_end = A.End();
		integer max = 1,min=0;
		while( it != it_end ){
			//      cerr<<"it="<<(*it)<<endl;
			if (max < (*it))
				max = *it;
			if ( min > (*it))
				min = *it;
			it++;
		}
		if (max<-min)
			max=-min;
		size_t n=A.coldim();
		double hadamarcp = n/2.0*(log(double(n))+2*log(double(max))+0.21163275)/log(2.0);

		Cra< CraBuilderFullMultip<Field > > cra(hadamarcp);
#endif
		Cra< CraBuilderEarlyMultip<Field > > cra(3UL);

		IntegerModularCharpoly<Blackbox,Method::Blackbox> iteration(A, M);
		cra.operator() (P, iteration, genprime);
		commentator().stop ("done", NULL, "IbbCharpoly");
#ifdef _LB_CRATIMING
        cra.reportTimes(std::clog);
#endif
		return P;
	}


	template <class Blackbox, class Polynomial>
	Polynomial& charpoly (Polynomial                       & P,
			      const Blackbox                   & A,
			      const RingCategories::IntegerTag & tag,
			      const Method::DenseElimination    & M)
	{
		if (A.coldim() != A.rowdim())
			throw LinboxError("LinBox ERROR: matrix must be square for characteristic polynomial computation\n");

		commentator().start ("Integer Dense Charpoly : No NTL installation -> chinese remaindering", "IbbCharpoly");

        typedef Givaro::ModularBalanced<double> Field;
		PrimeIterator<IteratorCategories::HeuristicTag> genprime(FieldTraits<Field>::bestBitSize(A.coldim()));
#if 0
		typename Blackbox::ConstIterator it = A.Begin();
		typename Blackbox::ConstIterator it_end = A.End();
		integer max = 1,min=0;
		while( it != it_end ){
			if (max < (*it))
				max = *it;
			if ( min > (*it))
				min = *it;
			it++;
		}
		if (max<-min)
			max=-min;
		size_t n=A.coldim();
		double hadamarcp = n/2.0*(log(double(n))+2*log(double(max))+0.21163275)/log(2.0);


		Cra< CraBuilderFullMultip<Field > > cra(hadamarcp);
#endif
		Cra< CraBuilderEarlyMultip<Field > > cra(3UL);
        IntegerModularCharpoly<Blackbox,Method::DenseElimination> iteration(A, M);
		cra (P, iteration, genprime);
		commentator().stop ("done", NULL, "IbbCharpoly");
#ifdef _LB_CRATIMING
        cra.reportTimes(std::clog);
#endif
		return P;
	}

}

#endif

namespace LinBox
{
	/** Compute the characteristic polynomial over \f$\mathbf{Z}_p\f$.
	 *
	 * Compute the characteristic polynomial of a matrix, represented via
	 * a blackBox.
	 *
	 * @param P Polynomial where to store the result
	 * @param A Blackbox representing the matrix
	 * @param tag
	 * @param M
	 */
	template <class Blackbox, class Polynomial/*, class Categorytag*/ >
	Polynomial& charpoly (Polynomial                       & P,
			      const Blackbox                   & A,
			      const RingCategories::ModularTag & tag,
			      const Method::Blackbox           & M)
	{
		if (A.coldim() != A.rowdim())
			throw LinboxError("LinBox ERROR: matrix must be square for characteristic polynomial computation\n");

		return BBcharpoly::blackboxcharpoly (P, A, tag, M);
	}

	template < class Blackbox, class Polynomial, class MyMethod>
	Polynomial &charpoly (Polynomial& P, const Blackbox& A,
			      const RingCategories::RationalTag& tag, const MyMethod& M)
	{
		commentator().start ("Rational Charpoly", "Rcharpoly");

        typedef Givaro::ModularBalanced<double> Field;
		PrimeIterator<IteratorCategories::HeuristicTag> genprime(FieldTraits<Field>::bestBitSize(A.coldim()));
		RationalRemainder2< VarprecCraBuilderEarlyMultip<Field > > rra(3UL);
		IntegerModularCharpoly<Blackbox,MyMethod> iteration(A, M);

		Givaro::ZRing<Integer> Z;
        DensePolynomial<Givaro::ZRing<Integer> > PP(Z); // use of integer due to non genericity of cra. PG 2005-08-04
		Integer den;
		rra(dynamic_cast<decltype(PP)::Domain_t::Storage_t&>(PP),den, iteration, genprime);
		size_t i =0;
		P.resize(PP.size());
		for (typename Polynomial::iterator it= P.begin(); it != P.end(); ++it, ++i)
			A.field().init(*it, PP[i],den);

		commentator().stop ("done", NULL, "Rcharpoly");

		return P;
	}

}  // end of LinBox namespace
#endif // __LINBOX_charpoly_H

// Local Variables:
// mode: C++
// tab-width: 4
// indent-tabs-mode: nil
// c-basic-offset: 4
// End:
// vim:sts=4:sw=4:ts=4:et:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s
