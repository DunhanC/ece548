#include <random>

class Perceptron {
public:
    Perceptron (int attr, int ord, double lf) {
        learning_sessions = 0;
        num_attr = attr;
        order = ord;
        learning_factor = lf;

        // The number of weights to be trained is 1 + (n choose r).
        // For example, if the attribute vector is of length 3 and we are
        // searching for a linear solution, then there will be 4 weights.
        num_weights = n_choose_r(num_attr + ord, ord);
        weights = random_weights(num_weights);
        map = construct_map(num_weights);
    }

    ~Perceptron () {
        delete[] weights;
        for (int i = 0; i < num_weights; i++)
            delete[] map[i];
        delete[] map;
    }

    void learn (const double attr_vector[], bool classification) {
        learning_sessions++;

        bool hypothesis = sum(attr_vector) > 0;

        // If perceptron was correct, there is no need to update weights.
        if (hypothesis == classification) return;

        // Since this part of the method is guarded against h(x) = c(x),
        // if h(x) is true, then c(x) must be false, so the weights should
        // be pulled downwards. Conversely, if h(x) is false, c(x) must be true,
        // so the weights should be pulled upwards.
        double adjustment = hypothesis ? -learning_factor : learning_factor;

        for (int i = 0; i < num_weights; i++) {
            weights[i] += adjustment * weight_inclusion(attr_vector, i);
        }
    }

    bool test (const double attr_vector[]) {
        return sum(attr_vector) > 0;
    }

    int num_weights () { return num_weights; }
    double* get_weights () { return weights; }

private:
    int num_attr;
    int order;
    double learning_factor;
    long learning_sessions;
    int num_weights;
    double *weights;
    unsigned **map;

    /*
        Since Perceptron will handle polynomials of arbitrary degree*, we will
        need to construct a map to tell us which weights correspond to which
        attributes. The trick is to use triangular numbers to break down the
        polynomial of order N into polynomials of order 0, 1, 2, ..., N.

        For example, x1 * x1 would be represented as { 2, 1, 1 } since it is of
        order 2 and it represents the first attribute times the first attribute.
        Similarly, x1 * x2 would be represented as { 2, 1, 2 }.

        The method includes a detailed explanation in the body. For a different
        explanation, see project1/map_explanation.txt.

        * in this case, only up to 28 because those are the only trianglular
        numbers I calculated and because 28 itself is already excessive.
    */
    unsigned ** construct_map (int length) {
        int triangle_numbers[28] = {
            1, 3, 6, 10, 15,
            21, 28, 36, 45, 55,
            66, 78, 91, 105, 120,
            136, 153, 171, 190, 210,
            231, 253, 276, 300, 325,
            351, 378, 406
        };
        unsigned ** map = new unsigned*[length];

        int seq_len = 0;
        int encountered = 1;

        map[0] = new unsigned [2]();
        map[0][0] = 1;

        while (encountered < length) {
            if (encountered >= triangle_numbers[seq_len])
                seq_len += 1;

            /*
                We need to count the number of non-strictly-increasing sequences
                of polynomials of a given order.

                For example, consider order=2 and variables=3, then the
                following is an enumeration of all the options:

                    xx, xy, xz, yy, yz, zz

                Note that there are 6 combinations. This is not a coincidence.
                The 3rd triangle number is 6.

                To generate these polynomials, we will first generate
                `num_polynomials` arrays of length `1 + seq_len`, all of which
                will be of the form
                    { seq_len, 1, 1, 1, ..., 1 }
                where there are `seq_len` 1s.
            */

            int num_polynomials = triangle_numbers[seq_len];
            for (int i = 0; i < num_polynomials; i++) {
                map[encountered + i] = new unsigned [1 + seq_len];
                map[encountered + i][0] = seq_len;
                for (int j = 1; j <= seq_len; j++)
                    map[encountered + i][j] = 1;
            }

            /*
                Next, we will roll through the arrays like a slot machine.
                Considering the same example as before (order=2, variables=3),
                represent the polynomials as numbers:

                    11, 12, 13, 22, 23, 33

                Think of them as numbers in base 3. We can easily find the next
                polynomial by adding 1 to the last digit. But since we're
                working in base 3, if that last digit is equal to 3, then we
                roll the addition over to the previous digit. That is, 13 would
                go to 22, not 14. Since the sequence is monotonic, the trick is
                to make sure any digits which follow the one you increased are
                equal to the digit you changed.

                This method is implemented as `increase_array`.
            */

            for (int i = 1; i < num_polynomials; i++) {
                for (int j = 1; j <= seq_len; j++)
                    map[encountered + i][j] = map[encountered + i - 1][j];
                increase_array(map[encountered + i], seq_len, 1 + seq_len, order);
            }

            encountered += num_polynomials;
        }

        // for (int i = 0; i < length; i++) {
        //     std::cout << "i=" << i << " j=" << map[i][0] << ": ";
        //     for (int j = 1; j <= map[i][0]; j++) {
        //         std::cout << map[i][j] << " ";
        //     }
        //     std::cout << std::endl;
        // }

        return map;
    }

    /*
        Initialize weights as an array of doubles from (0, 1). See
        http://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
        for further details.
    */
    double * random_weights (int length) {
        double * w = new double[length];

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        for (int i = 0; i < length; i++)
            w[i] = dis(gen);

        return w;
    }

    double weight_inclusion (const double attr_vector[], int index) {
        /*
            Recall that map is of the form

                { array_length, attrib_loc1, attrib_loc2, ... }

            where attrib_loc[i] is in index-1 notation.

            weights[i] corresponds to the conjunction (i.e., AND operation)
            of the attributes located at these indices.

            For example, if we're working with order=2 and attributes=3, then
            map[5] = { 2, 1, 2 } to represent `xy`. So we want to return x * y.
            In this case, that is represented by attrib[0] * attrib[1].

            Simply perform the product specified by map[index].
        */
        double product = 1.0;

        // The constant (w0) is always included in learning.
        if (index == 0) return 1.0;

        for (int i = 1; i <= map[index][0]; i++) {
            int attrib_index = map[index][i] - 1;
            product *= attr_vector[attrib_index];
        }

        return product;
    }

    double sum (const double attr_vector[]) {
        double total = weights[0];

        for (int i = 1; i < num_weights; i++) {
            total += weights[i] * weight_inclusion(attr_vector, i);
        }

        return total;
    }

    static void increase_array (unsigned *arr, int offset, int length, int max) {
        if (arr[offset] == max)
            increase_array(arr, offset - 1, length, max);
        else {
            arr[offset] += 1;
            for (int i = offset; i < length; i++)
                arr[i] = arr[offset];
        }
    }

    static unsigned n_choose_r (unsigned n, unsigned r) {
        if (r > n) return 0;
        if (r == 0) return 1;
        if (r * 2 > n) r = n - r;

        int result = n;

        for (int i = 2; i <= r; i++) {
            result *= (n - i + 1);
            result /= i;
        }

        return result;
    }
};
