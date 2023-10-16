namespace dtCore {

template <typename TrajType> void dtTrajectoryList<TrajType>::Clear() {
  m_trajList.clear();
}

template <typename TrajType>
void dtTrajectoryList<TrajType>::Add(const typename TrajType::ValType t,
                                     const TrajType &traj) {
  Cont cont(t, traj);

  typename std::list<Cont>::iterator itr = m_trajList.begin();
  while (itr != m_trajList.end()) {
    if ((*itr) > cont) {
      // insert before
      m_trajList.insert(itr, cont);
      return;
    }
    itr++;
  }
  m_trajList.push_back(cont);
}

template <typename TrajType>
void dtTrajectoryList<TrajType>::Interpolate(
    const typename TrajType::ValType t, typename TrajType::ContRefType p,
    typename TrajType::ContRefType v, typename TrajType::ContRefType a) const {
    // typename TrajType::ValType t0;
    // TrajType traj;
    // GetAt(t, t0, traj);

  typename std::list<Cont>::const_iterator itr = m_trajList.begin();
  if (itr != m_trajList.end()) {
    typename TrajType::ValType t0 = (*itr).key();
    TrajType &&traj = (*itr).value();
    traj.Interpolate(t - t0, p, v, a);
  }
}

template <typename TrajType>
void dtTrajectoryList<TrajType>::Interpolate(
    const typename TrajType::ValType t,
    typename TrajType::ContRefType p) const {}

template <typename TrajType>
void dtTrajectoryList<TrajType>::GetAt(const typename TrajType::ValType t,
                                       typename TrajType::ValType &t0,
                                       TrajType &traj) {
  typename std::list<Cont>::iterator itr = m_trajList.begin();

  if (itr != m_trajList.end()) 
  {
    t0 = (*itr).key();
    traj = (*itr).value();
  }

  while (itr != m_trajList.end()) 
  {
    if ((*itr) < t) {
      t0 = (*itr).key();
      traj = (*itr).value();
    } else {
      break;
    }
    itr++;
  }
}

} // namespace dtCore
