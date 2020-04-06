//   OpenNN: Open Neural Networks Library
//   www.opennn.net
//
//   G R A D I E N T   D E S C E N T   C L A S S   H E A D E R
//
//   Artificial Intelligence Techniques SL
//   artelnics@artelnics.com

#ifndef GRADIENTDESCENT_H
#define GRADIENTDESCENT_H

// System includes

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <functional>
#include <limits>
#include <limits.h>
#include <cmath>
#include <ctime>

// OpenNN includes

#include "loss_index.h"

#include "optimization_algorithm.h"
#include "learning_rate_algorithm.h"
#include "config.h"


namespace OpenNN
{

/// The process of making changes to weights and biases,
/// where the changes are propotyional to derivatives of network error with respect to those weights and biases.
/// This is done to minimize network error.

/// This concrete class represents the gradient descent optimization algorithm[1], used to minimize loss function.
///
/// \cite 1  Neural Designer "5 Algorithms to Train a Neural Network."
/// \ref https://www.neuraldesigner.com/blog/5_algorithms_to_train_a_neural_network

class GradientDescent : public OptimizationAlgorithm
{

public:

    struct OptimizationData
    {
        /// Default constructor.

        explicit OptimizationData()
        {
        }

        explicit OptimizationData(GradientDescent* new_gradient_descent_pointer)
        {
            set(new_gradient_descent_pointer);
        }

        virtual ~OptimizationData() {}

        void set(GradientDescent* new_gradient_descent_pointer)
        {
            gradient_descent_pointer = new_gradient_descent_pointer;

            LossIndex* loss_index_pointer = gradient_descent_pointer->get_loss_index_pointer();

            NeuralNetwork* neural_network_pointer = loss_index_pointer->get_neural_network_pointer();

            const Index parameters_number = neural_network_pointer->get_parameters_number();

            // Neural network data

            parameters.resize(parameters_number);
            parameters = neural_network_pointer->get_parameters();

            old_parameters.resize(parameters_number);

            parameters_increment.resize(parameters_number);

            // Loss index data

            old_gradient.resize(parameters_number);

            // Optimization algorithm data

            training_direction.resize(parameters_number);

        }

        void print() const
        {
            cout << "Training Direction:" << endl;
            cout << training_direction << endl;

            cout << "Learning rate:" << endl;
            cout << learning_rate << endl;

            cout << "Parameters:" << endl;
            cout << parameters << endl;
        }

        GradientDescent* gradient_descent_pointer = nullptr;

        // Neural network data

        Tensor<type, 1> parameters;
        Tensor<type, 1> old_parameters;

        Tensor<type, 1> parameters_increment;

        type parameters_increment_norm = 0;

        // Loss index data

        type old_training_loss = 0;

        Tensor<type, 1> old_gradient;

        Tensor<type, 2> inverse_hessian;
        Tensor<type, 2> old_inverse_hessian;

        // Optimization algorithm data

        Index epoch = 0;

        Tensor<type, 1> training_direction;

        Tensor<type, 0> training_slope;

        type learning_rate = 0;
        type old_learning_rate = 0;
    };

   // Constructors

   explicit GradientDescent(); 

   explicit GradientDescent(LossIndex*);

   explicit GradientDescent(const tinyxml2::XMLDocument&); 

   // Destructor

   virtual ~GradientDescent();   


   const LearningRateAlgorithm& get_learning_rate_algorithm() const;
   LearningRateAlgorithm* get_learning_rate_algorithm_pointer();

   // Training parameters

   const type& get_warning_parameters_norm() const;
   const type& get_warning_gradient_norm() const;
   const type& get_warning_learning_rate() const;

   const type& get_error_parameters_norm() const;
   const type& get_error_gradient_norm() const;
   const type& get_error_learning_rate() const;

   // Stopping criteria

   const type& get_minimum_parameters_increment_norm() const;

   const type& get_minimum_loss_decrease() const;
   const type& get_loss_goal() const;
   const type& get_gradient_norm_goal() const;
   const Index& get_maximum_selection_error_increases() const;

   const Index& get_maximum_epochs_number() const;
   const type& get_maximum_time() const;

   const bool& get_choose_best_selection() const;
   const bool& get_apply_early_stopping() const;

   // Reserve training history

   const bool& get_reserve_training_error_history() const;
   const bool& get_reserve_selection_error_history() const;

   // Set methods

   void set_loss_index_pointer(LossIndex*);

   void set_learning_rate_algorithm(const LearningRateAlgorithm&);

   void set_default();

   void set_reserve_all_training_history(const bool&);

   // Training parameters

   void set_warning_parameters_norm(const type&);
   void set_warning_gradient_norm(const type&);
   void set_warning_learning_rate(const type&);

   void set_error_parameters_norm(const type&);
   void set_error_gradient_norm(const type&);
   void set_error_learning_rate(const type&);

   void set_maximum_epochs_number(const Index&);

   // Stopping criteria

   void set_minimum_parameters_increment_norm(const type&);

   void set_minimum_loss_decrease(const type&);
   void set_loss_goal(const type&);
   void set_gradient_norm_goal(const type&);
   void set_maximum_selection_error_increases(const Index&);

   void set_maximum_time(const type&);

   void set_choose_best_selection(const bool&);
   void set_apply_early_stopping(const bool&);

   // Reserve training history

   void set_reserve_training_error_history(const bool&);
   void set_reserve_selection_error_history(const bool&);

   // Utilities

   void set_display_period(const Index&);

   // Training methods

   Tensor<type, 1> calculate_training_direction(const Tensor<type, 1>&) const;

   Results perform_training();

   void perform_training_void();

   string write_optimization_algorithm_type() const;

   // Serialization methods

   Tensor<string, 2> to_string_matrix() const;

   tinyxml2::XMLDocument* to_XML() const;
   void from_XML(const tinyxml2::XMLDocument&);

   void write_XML(tinyxml2::XMLPrinter&) const;

   void update_epoch(
           const DataSet::Batch& batch,
           NeuralNetwork::ForwardPropagation& forward_propagation,
           const LossIndex::BackPropagation& back_propagation,
           OptimizationData& optimization_data)
   {

       optimization_data.training_direction = calculate_training_direction(back_propagation.gradient);

       if(l2_norm(optimization_data.training_direction) < numeric_limits<type>::min())
           throw logic_error("Training direction is zero");

       // Training slope

       optimization_data.training_slope = normalized(back_propagation.gradient).contract(optimization_data.training_direction, AT_B);

       if(optimization_data.training_slope(0) >= static_cast<type>(0.0))
           throw logic_error("Training slope is equal or greater than zero");

       // Get initial learning_rate

       type initial_learning_rate = 0;

       optimization_data.epoch == 0
               ? initial_learning_rate = first_learning_rate
               : initial_learning_rate = optimization_data.old_learning_rate;

       pair<type,type> directional_point = learning_rate_algorithm.calculate_directional_point(
                               batch,
                               optimization_data.parameters,
                               forward_propagation,
                               back_propagation.loss,
                               optimization_data.training_direction,
                               initial_learning_rate);

       optimization_data.learning_rate = directional_point.first;

       if(abs(optimization_data.learning_rate) < numeric_limits<type>::min())
           throw logic_error("Training rate is zero");

       optimization_data.parameters_increment = optimization_data.training_direction*optimization_data.learning_rate;

       optimization_data.old_parameters = optimization_data.parameters;

       optimization_data.parameters += optimization_data.parameters_increment;

       optimization_data.old_gradient = back_propagation.gradient;

       optimization_data.parameters_increment_norm = l2_norm(optimization_data.parameters_increment);

       optimization_data.old_learning_rate = optimization_data.learning_rate;

       optimization_data.old_training_loss = back_propagation.loss;
   }

private:

   // TRAINING OPERATORS

   /// Training rate algorithm object for one-dimensional minimization. 

   LearningRateAlgorithm learning_rate_algorithm;

   type first_learning_rate = static_cast<type>(0.01);

   // TRAINING PARAMETERS

   /// Value for the parameters norm at which a warning message is written to the screen. 

   type warning_parameters_norm;

   /// Value for the gradient norm at which a warning message is written to the screen. 

   type warning_gradient_norm;

   /// Training rate value at wich a warning message is written to the screen.

   type warning_learning_rate;

   /// Value for the parameters norm at which the training process is assumed to fail. 
   
   type error_parameters_norm;

   /// Value for the gradient norm at which the training process is assumed to fail. 

   type error_gradient_norm;

   /// Training rate at wich the line minimization algorithm is assumed to be unable to bracket a minimum.

   type error_learning_rate;

   // Stopping criteria

   /// Norm of the parameters increment vector at which training stops.

   type minimum_parameters_increment_norm;

   /// Minimum loss improvement between two successive iterations. It is used as a stopping criterion.

   type minimum_loss_decrease;

   /// Goal value for the loss. It is used as a stopping criterion.

   type training_loss_goal;

   /// Goal value for the norm of the error function gradient. It is used as a stopping criterion.

   type gradient_norm_goal;

   /// Maximum number of iterations at which the selection error increases.
   /// This is an early stopping method for improving selection.

   Index maximum_selection_error_increases;

   /// Initial batch size

   Index training_initial_batch_size;

   /// Maximum training batch size

   Index training_maximum_batch_size;

   /// Maximum epochs number

   Index maximum_epochs_number;

   /// Maximum training time. It is used as a stopping criterion.

   type maximum_time;

   /// True if the final model will be the neural network with the minimum selection error, false otherwise.

   bool choose_best_selection;

   /// True if the selection error decrease stopping criteria has to be taken in account, false otherwise.

   bool apply_early_stopping;

   // TRAINING HISTORY 

   /// True if the loss history vector is to be reserved, false otherwise.

   bool reserve_training_error_history;

   /// True if the selection error history vector is to be reserved, false otherwise.

   bool reserve_selection_error_history;
};

}

#endif
