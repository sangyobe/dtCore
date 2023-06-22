
inline Scalar at(uint16_t i, uint16_t j) const { return this->operator()(i, j); }
inline Scalar &at(uint16_t i, uint16_t j) { return this->operator()(i, j); }
inline Scalar at(uint16_t i) const { return this->operator[](i); }
inline Scalar &at(uint16_t i) { return this->operator[](i); }

// const CwiseUnaryOp<internal::scalar_add_op<Scalar>, Derived>
// operator+(const Scalar& scalar) const
// { return CwiseUnaryOp<internal::scalar_add_op<Scalar>, Derived>(derived(),
// internal::scalar_add_op<Scalar>(scalar)); }

// friend const CwiseUnaryOp<internal::scalar_add_op<Scalar>, Derived>
// operator+(const Scalar& scalar, const MatrixBase<Derived>& mat)
// { return CwiseUnaryOp<internal::scalar_add_op<Scalar>,
// Derived>(mat.derived(), internal::scalar_add_op<Scalar>(scalar)); }

// Vector3d
inline void setXYZ(const Scalar &x_, const Scalar &y_, const Scalar &z_) {
  assert(size() == 3);
  this->x() = x_;
  this->y() = y_;
  this->z() = z_;
}

// dVector
/**
 * set the elements of the index set
 * @param perm the index set
 * @param rhs the source vector
 */
template <typename OtherDerived>
inline void set(const std::vector<long> &perm,
                const Eigen::MatrixBase<OtherDerived> &rhs) {
  assert(rhs.size() >= perm.size());
  for (Index k = 0; k < perm.size(); k++)
    this->operator[](perm[k]) = rhs[k];
}
// dMatrix
/**
 * set the elements of the submatrix by the row and column index sets by the
 * matrix
 * @param rperm the row index set
 * @param cperm the column index set
 * @param rhs the source matrix
 */
template <typename OtherDerived>
inline void set(const std::vector<long> &rperm, const std::vector<long> &cperm,
                const Eigen::MatrixBase<OtherDerived> &rhs) {
  assert(rhs.rows() >= rperm.size() && rhs.cols() >= cperm.size());
  for (uint16_t j = 0; j < cperm.size(); j++)
    for (uint16_t i = 0; i < rperm.size(); i++)
      this->operator()(rperm[i], cperm[j]) = rhs(i, j);
}
/**
 * set the elements of the column submatrix by the row index set by the matrix
 * @param rperm the row index set
 * @param c the head column index of the submatrix
 * @param n the number of columns of the submatrix
 * @param rhs the source matrix
 */
template <typename OtherDerived>
inline void set(const std::vector<long> &rperm, const long &c, const long &n,
                const Eigen::MatrixBase<OtherDerived> &rhs) {
  assert(cols() >= c + n);
  assert(rhs.rows() >= rperm.size() && rhs.cols() >= n);
  for (uint16_t j = 0; j < n; j++)
    for (uint16_t i = 0; i < rperm.size(); i++)
      this->operator()(rperm[i], c + j) = rhs(i, j);
}
/**
 * set the elements of the row submatrix by the column index set by a matrix
 * @param r the head row index of the submatrix
 * @param m the number of rows of the submatrix
 * @param cperm the column index set
 * @param rhs the source matrix
 */
template <typename OtherDerived>
inline void set(const long &r, const long &m, const std::vector<long> &cperm,
                const Eigen::MatrixBase<OtherDerived> &rhs) {
  assert(rows() >= r + m);
  assert(rhs.rows() >= m && rhs.cols() >= cperm.size());
  for (uint16_t j = 0; j < cperm.size(); j++)
    for (uint16_t i = 0; i < m; i++)
      this->operator()(r + i, cperm[j]) = rhs(i, j);
}

// dMatrix
template <typename OtherDerived>
EIGEN_STRONG_INLINE void set(const long &i, const long &j, const long &m,
                             const long &n,
                             const Eigen::MatrixBase<OtherDerived> &rhs) {
  assert(rhs.rows() == m && rhs.cols() == n);
  Base::block(i, j, m, n) = rhs;
} // KCX //???

// dVector
template <typename OtherDerived>
EIGEN_STRONG_INLINE void set(const Index &i, const Index &n,
                             const Eigen::MatrixBase<OtherDerived> &rhs) {
  assert(rhs.size() == n);
  this->segment(i, n) = rhs.head(n);
} // KCXY //???

// dVector
/**
 * construct the vector (of size n), initialized by float array
 * @param n dimension of the vector (n > 0)
 * @param rhs 1-d array of float (of size greater than n)
 */
template <typename Type> inline void set(const Type *values) {
  for (Index k = 0; k < size(); k++)
    this->operator[](k) = values[k];
}

// inline bool isValidRotation(Scalar threshold = 1e-4)
//{
//	assert(rows() == cols() == 3);
//
//	uint16_t i;
//	for (i = 0; i < 3; i++)
//	{
//		if (std::fabs(col(i).dot(col(i)) - 1) >= threshold) break; //
//normal 		if (std::fabs(col(i).dot(col((i + 1)%3))) >= threshold) break; //
//orhogonal
//	}
//	if (i == 3 && std::fabs(determinant() - 1) < threshold) return true;
//	ucout << "Rotation::Set() is not valid rotation" << std::endl;
//	return false;
// }
/**
 * solve the square linear equation. Note that the self matrix is overwritten.
 * @param rhs the data. On output, it has the solution, i.e. rhs = self^{-1}*rhs
 * @return the error code
 */
// self: mxm, invertable, general or symmetric positive definite
template <typename OtherDerived>
inline void solve(Eigen::MatrixBase<OtherDerived> &mat,
                  const uint16_t type = 0 /* GENERAL */) const {
  if (type == 0)
    mat = lu().solve(mat);
  else /* if (type == POSITIVEDEFINITE) */
    mat = llt().solve(mat);
} // KCX //???
