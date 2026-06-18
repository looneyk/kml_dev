
    //--------------------------------------------------------------------------
    size_t balanced_binary_tree_node_count(size_t breadth)
    {
        if (breadth == 0)
        {
            return 0;
        }

        size_t n = 0;
        for (size_t full = breadth * 2 - 1; full > 0; full /= 2)
        {
            ++n;
        }

        return n;
    }
