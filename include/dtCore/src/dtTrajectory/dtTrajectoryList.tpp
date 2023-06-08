namespace dtCore {

template <typename TrajType>
void dtTrajectoryList<TrajType>::add(const TrajType &traj) {}

template <typename TrajType>
void dtTrajectoryList<TrajType>::interpolate(
    double t, typename TrajType::ContainerType &p,
    typename TrajType::ContainerType &v,
    typename TrajType::ContainerType &a) const {}

template <typename TrajType>
void dtTrajectoryList<TrajType>::interpolate(
    double t, typename TrajType::ContainerType &p) const {}

} // namespace dtCore
