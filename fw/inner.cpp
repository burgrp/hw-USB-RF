namespace utils
{

template <class T>
class Inner
{
public:
    T *outer;
    void bind(T *outer)
    {
        this->outer = outer;
    }
};

} // namespace utils