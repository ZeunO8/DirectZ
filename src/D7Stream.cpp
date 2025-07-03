namespace dz {
    size_t* D7Stream::addStreamPoint(const StreamPoint& point)
    {
        auto index_ptr = new size_t(stream_points.size());
        stream_points.push_back(point);
        stream_indexes.push_back(index_ptr);
        current_point_index = index_ptr;
        return index_ptr;
    }

    bool D7Stream::removeStreamPoint(size_t* index_ptr) {
        if (!index_ptr)
            return false;
        if (current_point_index == index_ptr) {
            if (stream_indexes.size() > 1) {
                if (*index_ptr > 0) {
                    current_point_index = stream_indexes[(*index_ptr) - 1];
                }
                else {
                    current_point_index = stream_indexes[(*index_ptr)];
                }
            }
            else {
                current_point_index = 0;
            }
        }
        stream_points.erase(stream_points.begin() + *index_ptr);
        stream_indexes.erase(stream_indexes.begin() + *index_ptr);
        auto stream_points_size = stream_points.size();
        for (auto i = (*index_ptr); i < stream_points_size; i++)
        {
            --(*stream_indexes[i]);
        }
        delete index_ptr;
        return true;
    }

    bool D7Stream::rewindNPoints(size_t N, D7Type filter, const StreamString& a_buff, StreamInteger uid, StreamIdentifier Uid)
    {
        if (!current_point_index || *current_point_index == 0)
            return false;

        size_t current = *current_point_index;
        size_t moved = 0;

        while (current > 0 && moved < N)
        {
            --current;
            const auto& point = stream_points[current];
            bool match = true;

            if ((filter & D7Type::a) && std::get<D7TypeToIndex<D7Type::a>()>(point) != a_buff)
                match = false;
            if ((filter & D7Type::u) && std::get<D7TypeToIndex<D7Type::u>()>(point) != uid)
                match = false;
            if ((filter & D7Type::U) && std::get<D7TypeToIndex<D7Type::U>()>(point) != Uid)
                match = false;

            if (match)
                ++moved;
        }

        current_point_index = stream_indexes[current];
        return true;
    }

    bool D7Stream::forwardNPoints(size_t N, D7Type filter, const StreamString& a_buff, StreamInteger uid, StreamIdentifier Uid)
    {
        if (!current_point_index || *current_point_index >= stream_points.size() - 1)
            return false;

        size_t current = *current_point_index;
        size_t moved = 0;
        const size_t max_index = stream_points.size() - 1;

        while (current < max_index && moved < N)
        {
            ++current;
            const auto& point = stream_points[current];
            bool match = true;

            if ((filter & D7Type::a) && std::get<D7TypeToIndex<D7Type::a>()>(point) != a_buff)
                match = false;
            if ((filter & D7Type::u) && std::get<D7TypeToIndex<D7Type::u>()>(point) != uid)
                match = false;
            if ((filter & D7Type::U) && std::get<D7TypeToIndex<D7Type::U>()>(point) != Uid)
                match = false;

            if (match)
                ++moved;
        }

        current_point_index = stream_indexes[current];
        return true;
    }

    void D7Stream::printStreamPoints(D7Type filter, const StreamString& a_buff, StreamInteger uid, StreamIdentifier Uid)
    {
        std::cout << "\n=== Filtered Stream Output ===\n";
        for (size_t i = 0; i < stream_points.size(); ++i)
        {
            const auto& point = stream_points[i];
            bool match = true;

            if ((filter & D7Type::a) && std::get<D7TypeToIndex<D7Type::a>()>(point) != a_buff)
                match = false;
            if ((filter & D7Type::u) && std::get<D7TypeToIndex<D7Type::u>()>(point) != uid)
                match = false;
            if ((filter & D7Type::U) && std::get<D7TypeToIndex<D7Type::U>()>(point) != Uid)
                match = false;

            if (!match)
                continue;

            auto x = std::get<D7TypeToIndex<D7Type::X>()>(point);
            auto y = std::get<D7TypeToIndex<D7Type::Y>()>(point);
            auto z = std::get<D7TypeToIndex<D7Type::Z>()>(point);
            auto t = std::get<D7TypeToIndex<D7Type::T>()>(point);
            auto uidVal = std::get<D7TypeToIndex<D7Type::u>()>(point);
            auto UidVal = std::get<D7TypeToIndex<D7Type::U>()>(point);
            auto a = std::get<D7TypeToIndex<D7Type::a>()>(point);

            std::cout << std::fixed << std::setprecision(2)
                    << "[" << i << "] Pos(" << x << ", " << y << ", " << z << ") "
                    << "UID: " << UidVal << " uid: " << uidVal << " action: " << a << "\n";
        }
    }

    D7Stream& D7Stream::operator<<(const StreamPoint& point) {
        addStreamPoint(point);
        return *this;
    }
}