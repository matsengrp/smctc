for i in $(seq 10); do
    for f in phyl*.cc phyl*.hh; do
      astyle -A$i < $f > A$i.$f
    done
done

