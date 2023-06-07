namespace dtCore {

template <typename m_type, dtTrajType m_trajType>
dtTrajectory<m_type, m_trajType>::dtTrajectory() : _t0(0), _tf(0) {}

template <typename m_type, dtTrajType m_trajType>
dtTrajectory<m_type, m_trajType>::~dtTrajectory() {}

template <typename m_type, dtTrajType m_trajType>
dtTrajectory<m_type, m_trajType>::dtTrajectory(const double t0, const double tf,
                                               const m_type &initial,
                                               const m_type &final)
    : _t0(t0), _tf(tf) {
#ifdef _DEBUG
  _p0 = initial;
  _pf = final;
#endif
  //_determineCoeff(initial, final);
}

template <typename m_type, dtTrajType m_trajType>
void dtTrajectory<m_type, m_trajType>::reconfigure(const double t0,
                                                   const double tf,
                                                   const m_type &initial,
                                                   const m_type &final) {
  _t0 = t0;
  _tf = tf;
#ifdef _DEBUG
  _p0 = initial;
  _pf = final;
#endif
  //_determineCoeff(initial, final);
}

} // namespace dtCore
