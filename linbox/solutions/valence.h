/* Copyright (C) 1999 LinBox
 * Written by <Jean-Guillaume.Dumas@imag.fr>
 * Modified by Z. Wan to fit in linbox
 *
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


// Givaro / Athapascan-1
// Valence computation

#ifndef __LINBOX_valence_H
#define __LINBOX_valence_H

#include "linbox/blackbox/transpose.h"

#include "linbox/solutions/minpoly.h"

namespace LinBox
{

	/*- @brief Valence of a blackbox linear operator A.
	 * This is the coefficient of the smallest degree
	 * non zero monomial of the minimal polynomial of A.
	 * The resulting value is a Field Element.
	 */
	template < class Blackbox, class DomainCategory, class MyMethod>
	typename Blackbox::Field::Element &valence (typename Blackbox::Field::Element & V,
						    const Blackbox& A,
						    const DomainCategory& tag,
						    const MyMethod& M);


	/** \brief Compute the valence of A
	 *
	 * The valence of a linear operator A, represented as a
	 * black box, is computed over the ring or field of A.
	 *
	 * @param v Field element into which to store the result
	 * @param A Black box of which to compute the determinant
	 * @param M may is a Method.
	 \ingroup solutions
	 */
	template <class Blackbox, class MyMethod>
	typename Blackbox::Field::Element &valence (typename Blackbox::Field::Element         &v,
						    const Blackbox                              &A,
						    const MyMethod                           &M)
	{
		return valence(v, A, typename FieldTraits<typename Blackbox::Field>::categoryTag(), M);
	}

	// The valence with default Method
	template<class Blackbox>
	typename Blackbox::Field::Element &valence (typename Blackbox::Field::Element         &v,
						    const Blackbox                               &A)
	{
		return valence(v, A, Method::Auto());
	}

	template<class Blackbox, class MyMethod>
	typename Blackbox::Field::Element &valence (
						    typename Blackbox::Field::Element         &v,
						    const Blackbox                            &A,
						    const RingCategories::ModularTag          &tag,
						    const MyMethod& M)
	{
		typedef typename Blackbox::Field Field;
		BlasVector<Field> minp(A.field());
		minpoly(minp, A, tag, M);
		typename BlasVector<Field>::const_iterator it = minp.begin();
		for( ; it != minp.end(); ++it)
			if (! A.field().isZero(*it)) break;
		if (it != minp.end())
			return v=*it;
		else
			return A.field().assign(v,A.field().zero);
	}

}

#include <givaro/modular.h>

#include "linbox/ring/modular.h"
#include "linbox/algorithms/cra-domain.h"
#include "linbox/algorithms/cra-builder-single.h"
#include "linbox/randiter/random-prime.h"
#include "linbox/algorithms/matrix-hom.h"

namespace LinBox
{

	template <class Blackbox, class MyMethod>
	struct IntegerModularValence {
		const Blackbox &A;
		const MyMethod &M;

		IntegerModularValence(const Blackbox& b, const MyMethod& n) :
			A(b), M(n)
		{}


		template<typename Field>
		IterationResult operator()(typename Field::Element& v, const Field& F) const
		{
			commentator().start ("Givaro::Modular Valence", "Mvalence");
// 			std::ostream& report = commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION);
// 			F.write(report) << std::endl;
			typedef typename Blackbox::template rebind<Field>::other FBlackbox;
// 			report << typeid(A).name() << ", A is: " << A.rowdim() << 'x' << A.coldim() << std::endl;

			FBlackbox Ap(A, F);

// 			report << typeid(Ap).name() << ", Ap is: " << Ap.rowdim() << 'x' << Ap.coldim() << std::endl;

			valence( v, Ap, M);
// 			F.write( F.write(report << "one valence: ", v) << " mod " ) << std::endl;;
			commentator().stop ("done", NULL, "Mvalence");
			return IterationResult::CONTINUE;
		}
	};

	template <class Blackbox, class MyMethod>
	typename Blackbox::Field::Element &valence (typename Blackbox::Field::Element &V,
						    const Blackbox                     &A,
						    const RingCategories::IntegerTag   &tag,
						    const MyMethod                     &M)
	{
		commentator().start ("Integer Valence", "Ivalence");
#if __SIZEOF_LONG__ == 8
		typedef Givaro::Modular<int64_t> Field;
#else
		typedef Givaro::ModularBalanced<double> Field;
#endif
                PrimeIterator<IteratorCategories::HeuristicTag> genprime(FieldTraits<Field>::bestBitSize(A.rowdim()));
		ChineseRemainder< CRABuilderEarlySingle<Field> > cra(LINBOX_DEFAULT_EARLY_TERMINATION_THRESHOLD);

		IntegerModularValence<Blackbox,MyMethod> iteration(A, M);
		cra(V, iteration, genprime);
		commentator().stop ("done", NULL, "Ivalence");
		return V;
	}


    template<class Blackbox>
    typename Blackbox::Field::Element &squarizeValence(
        typename Blackbox::Field::Element	&val_A,
        const Blackbox						&A,
        size_t method=0) {
            // method:	0 is automatic
            // 			1 is aat
            // 			2 is ata

        if (A.rowdim() == A.coldim()) {
            return valence(val_A, A);
        } else {
            if (! method) {
                if (A.rowdim() > A.coldim())
                    method=2;
                else
                    method=1;
            }
            Transpose<Blackbox> T(&A);
            if (method==2) {
                Compose< Transpose<Blackbox>, Blackbox > C (&T, &A);
                std::clog << "A^T A is " << C.rowdim() << " by " << C.coldim() << std::endl;
                return valence(val_A, C);
            } else {
                Compose< Blackbox, Transpose<Blackbox> > C (&A, &T);
                std::clog << "A A^T is " << C.rowdim() << " by " << C.coldim() << std::endl;
                return valence(val_A, C);
            }
        }
    }



} //End of LinBox


namespace LinBox
{


	class Valence {
	public:

		// compute the bound for eigenvalue of AAT via oval of cassini
		// works with both SparseMatrix and DenseMatrix
		template <class Blackbox>
		static integer& cassini (integer& r, const Blackbox& A)
		{
			//commentator().start ("Cassini bound", "cassini");
			integer _aat_diag, _aat_radius, _aat_radius1;
			typedef typename Blackbox::Field Ring;
			_aat_diag = 0; _aat_radius = 0, _aat_radius1 = 0;

			Givaro::ZRing<Integer> ZZ;
			BlasVector< Givaro::ZRing<Integer> > d(ZZ, A. rowdim()),w(ZZ, A. coldim());
			BlasVector<Givaro::ZRing<Integer>>::iterator di, wi;
			for(wi = w.begin();wi!= w.end();++wi)
				*wi = 0;
			for(di = d.begin();di!= d.end();++di)
				*di = 0;
			//typename Blackbox::ConstRowIterator row_p;
			typename Blackbox::Element tmp_e;
			Ring R(A. field());
			integer tmp; size_t i, j;

			for (j = 0, di = d. begin(); j < A. rowdim(); ++ j, ++ di) {
				// not efficient, but I am not tired of doing case by case
				for ( i = 0; i < A. coldim(); ++ i) {
					R. assign(tmp_e, A.getEntry( j, i));
					R. convert (tmp, tmp_e);
					if (tmp != 0) {
						*di += tmp * tmp;
						w [(size_t) i] += abs (tmp);
					}

					_aat_diag = _aat_diag >= *di ? _aat_diag : *di;
				}
			}

			for (j = 0, di = d. begin(); j < A. rowdim(); ++ j, ++ di) {
				integer local_radius = 0;
				for (i = 0; i < A. coldim(); ++ i) {
					R. assign (tmp_e, A. getEntry (j, i));
					R. convert (tmp, tmp_e);
					if (tmp != 0)
						local_radius += abs (tmp) * w[(size_t)i];
				}
				local_radius -= *di;
				if ( local_radius > _aat_radius1) {
					if ( local_radius > _aat_radius) {
						_aat_radius1 = _aat_radius;
						_aat_radius = local_radius;
					}
					else
						_aat_radius1 = local_radius;
				}
			}

			r = _aat_diag + (integer)sqrt( _aat_radius * _aat_radius1 );
			commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION);
			//std::cout << "Cassini bound (AAT) =: " << r << std::endl;
			//commentator().stop ("done", NULL, "cassini");
			return r;
		}

		// compute one valence of AAT over a field
		template <class Blackbox>
		static void one_valence(typename Blackbox::Element& v, size_t& r, const Blackbox& A)
		{
			//commentator().start ("One valence", "one valence");
			typedef BlasVector<typename Blackbox::Field> Poly;
			Poly poly(A.field());
			typename Blackbox::Field F(A. field());
			Transpose<Blackbox> AT (&A);
			Compose<Blackbox, Transpose<Blackbox> > AAT(&A, &AT);
			// compute the minpoly of AAT
			minpoly(poly, AAT, Method::Wiedemann());
			typename Poly::iterator p;
			F. assign(v, F.zero);

			for (p = poly. begin(); p != poly. end(); ++ p)
				if (! F. isZero (*p)) {
					F. assign (v, *p);
					break;
				}

			r = poly. size() -1;
			std::ostream& report = commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION);

			//	std::ostream& report = std::cout;
			report << "one valence =: " << v << " over ";
			A. field(). write(report); report << std::endl;
			//commentator().stop ("done", NULL, "one valence");
			return;
		}

		// compute the valence of AAT over an integer ring
		template <class Blackbox>
		static void valence(Integer& val, const Blackbox& A)
		{
			commentator().start ("Valence (AAT)", "Valence");
			typedef Givaro::Modular<int32_t> Field;
			typedef typename MatrixHomTrait<Blackbox, Field>::value_type FBlackbox;
			size_t d;
			PrimeIterator<IteratorCategories::HeuristicTag> g(FieldTraits<Field>::bestBitSize(A.coldim()));
			Field::Element v;
			++g;
			Field F((int32_t)*g);
			FBlackbox Ap(A, F);
			one_valence(v, d, Ap);
			commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION);
			//std::cout<<"degree of minpoly of AAT: " << d << std::endl;
			valence (val, d, A);
			commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION)
			<< "Integer valence =: " << val << std::endl;
			commentator().stop ("done", NULL, "Valence");
			return;
		}

		// compute the valence of AAT over an integer ring
		// d, the degree of min_poly of AAT
		template <class Blackbox>
		static void valence(Integer& val, size_t d, const Blackbox& A)
		{

			typedef Givaro::Modular<int32_t> Field;
			typedef typename MatrixHomTrait<Blackbox, Field>::value_type FBlackbox;

			PrimeIterator<IteratorCategories::HeuristicTag> rg(FieldTraits<Field>::bestBitSize(A.coldim()));
			Givaro::ZRing<Integer> Z;
			BlasVector<Givaro::ZRing<Integer> > Lv(Z), Lm(Z);
			size_t d1; Field::Element v; integer im = 1;
			//compute an upper bound for val.
			integer bound; cassini (bound, A); bound = pow (bound, (uint64_t)d); bound *= 2;
			commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION)
			<< "Bound for valence: " << bound << std::endl;

			do {
				++rg;
				Field F((uint64_t)*rg);
				FBlackbox Ap(A, F);
				one_valence(v, d1, Ap);
				if (d1 == d) {
					im *= *rg;
					Lm. push_back ( *rg ); Lv. push_back (integer(v));
				}
			} while (im < bound);

			val = 0;
			BlasVector<Givaro::ZRing<Integer> >::iterator Lv_p, Lm_p; integer tmp, a, b, g;
			for (Lv_p = Lv. begin(), Lm_p = Lm. begin(); Lv_p != Lv. end(); ++ Lv_p, ++ Lm_p) {
				tmp = im / *Lm_p;
				gcd (g, *Lm_p, tmp, a, b);
				val += *Lv_p * b * tmp;
				val %= im;
			}

			if (sign (val) < 0)
				val += im;
			tmp = val - im;
			if (abs(tmp) < abs(val))
				val = tmp;

			return;
		}
	};
} //End of LinBox
#endif //__LINBOX_valence_H

// Local Variables:
// mode: C++
// tab-width: 4
// indent-tabs-mode: nil
// c-basic-offset: 4
// End:
// vim:sts=4:sw=4:ts=4:et:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s
