for i in pad-oper unpad-paren keep-one-line-statements keep-one-line-blocks; do
    for f in phyl*.cc phyl*.hh; do
      echo "astyle --$i < $f > $i.$f"
    done
done


