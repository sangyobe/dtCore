namespace dtCore {

template <typename TrajType> void dtTrajectoryList<TrajType>::Clear() {
  m_trajList.clear();
}

template <typename TrajType>
void dtTrajectoryList<TrajType>::Add(const typename TrajType::ValType t,
                                     const TrajType &traj) {
  m_trajList.push_back(traj);
}

template <typename TrajType>
void dtTrajectoryList<TrajType>::Interpolate(
    const typename TrajType::ValType t, typename TrajType::ContRefType p,
    typename TrajType::ContRefType v, typename TrajType::ContRefType a) const {}

template <typename TrajType>
void dtTrajectoryList<TrajType>::Interpolate(
    const typename TrajType::ValType t,
    typename TrajType::ContRefType p) const {}

} // namespace dtCore
