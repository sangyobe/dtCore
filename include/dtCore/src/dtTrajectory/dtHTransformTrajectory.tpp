namespace dtCore {

template <typename ValueType>
dtHTransformTrajectory<ValueType>::dtHTransformTrajectory() {}

template <typename ValueType>
dtHTransformTrajectory<ValueType>::dtHTransformTrajectory(
    const ValueType t0, const ValueType tf, const ContRefType initial,
    const ContRefType final) {}

template <typename ValueType>
dtHTransformTrajectory<ValueType>::~dtHTransformTrajectory() {}

template <typename ValueType>
void dtHTransformTrajectory<ValueType>::Interpolate(const ValueType t,
                                                    ContRefType current) const {
}

template <typename ValueType>
void dtHTransformTrajectory<ValueType>::Reconfigure(const ContRefType initial,
                                                    const ContRefType final) {}

} // namespace dtCore