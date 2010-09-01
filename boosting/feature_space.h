/* feature_space.h                                                 -*- C++ -*-
   Jeremy Barnes, 16 February 2005
   Copyright (c) 2005 Jeremy Barnes.  All rights reserved.
   $Source$

   Feature space.  Provides a mapping between an application domain and the
   algorithms.
*/

#ifndef __boosting__feature_space_h__
#define __boosting__feature_space_h__

#include "config.h"
#include "feature_set.h"
#include "feature_info.h"

namespace ML {


enum Feature_Space_Type {
    DENSE,   ///< Dense feature space
    SPARSE   ///< Sparse feature space
};


/*****************************************************************************/
/* FEATURE_SPACE                                                             */
/*****************************************************************************/

/** This is a class that provides information on a space of features. */

class Feature_Space {
public:
    virtual ~Feature_Space();

    /** Return information on how we use a particular feature.
        \param feature       the Feature that we are interested in.  The
                             interpretation of this is up to the feature
                             space.
        \returns             a Feature_Info object for the given feature.
        
        This is an abstract method; subclasses must override it.  In most
        cases, choosing a REAL value should be sufficient.

        Note also that this method is called frequently, and thus shouldn't
        do any major computation.
     */
    virtual Feature_Info info(const Feature & feature) const = 0;


    /*************************************************************************/
    /* FEATURES                                                              */
    /*************************************************************************/

    /** \name Methods to deal with features
        By overriding these methods, the user of the library is able to
        control all aspects of how the features are manipulated and
        interpreted.

        These methods allow the feature space to specialise the interpretation
        and manipulation of features.  As all of these are done through the
        feature space, we can use the same feature type for all of the
        machine learning algorithms, thereby getting specialisation without
        having to either make Feature have virtual methods (which would make
        things much slower), or to use template parameters (which would make
        things much more complicated).

        By default, a feature is treated as 3 ints (the same as its internal
        representation).
        @{
    */

    /** Print a feature.  This method prints out a feature to an ASCII string.
        The restrictions on the format of the feature are:
        - Any colons and vertical bars (':' or '|') must be escaped with a
          backslash (and any backslashes escaped also as a double backslash).
        - If the first character is a quote, then the entire string is
          intrepreted as a quoted string.  In this case, any quotes and
          backslashes must themselves be backslashed (as in C++).
        - It cannot contain any CR or LF characters anywhere, no matter what.

        The reasons for these rules are that features are printed out using
        this method in various places in order to create parseable files, and
        breaking them would cause these files to not parse properly.

        \param feature       the feature that we want to print.  It is up to
                             the feature space to interpret this feature.
        \returns             a string representation of the feature.

        The method may throw an exception if the feature is unknown.  The
        default implementation of this method prints the feature out in the
        same way as the tuple<int, int, int> upon which it is based:

        \code
        Feature feature(1, 2, 3);
        boost::shared_ptr<Feature_Space> fs = ...;

        ...

        cerr << fs->Feature_Space::print(feature) << endl;
        \endcode

        \verbatim
        (1 2 3)
        \endverbatim
    */
    virtual std::string print(const Feature & feature) const;

    /** Print out a value of a feature.  This should print the way in which
        the feature space interprets the given value.  For non-categorical
        features, returning a string representation of the float is reasonable.

        For categorical features, it should print out whatever category is
        associated with the feature value.

        The default prints it out as a float.
    */
    virtual std::string print(const Feature & feature, float value) const;

    /** Parse a feature.  This method is called with a parse context pointing
        to the start of a segment of text which is assumed to have been
        generated by the \c print() method above.  Its job is to turn that
        into a tuple.

        In the case that the tuple can't be parsed, it should return \p false.
        In this case, the Parse_Context object should remain unmodified.  The
        Parse_Context::Token helper class is very useful for this.  It is not
        necessary to keep the feature parameter unmodified, however.

        \param context       the parse context pointing to the feature to be
                             parsed.
        \param feature       the Feature to be modified to the one read.
        \returns             \c true if a valid feature was found and parsed
                             or \c false otherwise.

        \post                <i>either</i> result is \c true and \p context
                             now points past the feature, <i>or</i> result
                             is /c false and \p context is unmodified.

        Note that this method should not throw an exception, even if the
        stream read is invalid (instead it should return false).  This is
        because the method is used in order to determine the end of a list of
        values:

        \code
        Parse_Context cxt(...);
        boost::shared_ptr<Feature_Space> fs(...);

        vector<Feature> features(1);
        while (fs->parse(cxt, feature.back())) feature.push_back(Feature());
        features.pop_back();
        \endcode

        The default implementation is capable of parsing the same
        representation as the default implementation of the print() method.
    */
    virtual bool parse(Parse_Context & context, Feature & feature) const;

    /** Parse the name of a feature. */
    virtual void parse(const std::string & name, Feature & feature) const;

    /** Expect a feature.  This method is very similar to \p print(), except
        that it is required to throw an exception if a valid feature cannot
        be found (in this case, \p context should not be modified).

        \param context       the parse context pointing to the feature to be
                             parsed.
        \param feature       the Feature to be modified to the one read.

        \post                <i>either</i> \p context now points past the
                             feature, <i>or</i> an exception was thrown and
                             \p context is unmodified.

        The default implementation implements this method in terms of
        print() using (roughly) the following logic:

        \code
        if (!parse(context, feature))
            throw Exception("Couldn't parse feature");
        \endcode
        
    */
    virtual void expect(Parse_Context & context, Feature & feature) const;

    /** Serialize a feature.  This method is called to write a feature to a
        binary store.

        \param store         the binary store to which the feature is written.
        \param feature       the feature to be written to the store.

        The default implementation simply serializes each of the 3 fields as
        a compact_size_t.
    */
    virtual void serialize(DB::Store_Writer & store,
                           const Feature & feature) const;

    /** Reconstitute a feature.  This method is called to read a feature from
        a binary store.

        \param store         the binary store to which the feature is read.
        \param feature       the feature to be modified by the store.

        The default implementation simply reconstitutes each of the 3 fields
        as a compact_size_t.  An exception should be thrown on any type of
        error---note that it is not really important that store remain
        unmodified in this case, as it is not usually possible to do any type
        of recovery from an error in binary reconstitution and the model
        doesn't support speculative reconstitution.
    */
    virtual void reconstitute(DB::Store_Reader & store,
                              Feature & feature) const;
    //@}

    /*************************************************************************/
    /* FEATURE VALUES                                                        */
    /*************************************************************************/

    /** \name Methods to deal with feature values

        For most features, the feature values are simply floats, and are
        serialized, reconstituted, printed and parsed as such.  For STRING
        features, however, the features are serialized/reconstitued directly
        as the string to avoid needing a consistent global mapping.
        @{
    */

    /** Serialize a feature value.  This method will write the given value of
        the given feature to a binary store.

        \param store         the binary store to which the feature is written.
        \param feature       the feature to which the value belongs.
        \param value         the feature value to be written.

        The default implementation serializes it as a float if the feature
        info gives anything but STRING, or as a string otherwise.

        Note that the feature itself is NOT serialized.
    */
    virtual void serialize(DB::Store_Writer & store,
                           const Feature & feature,
                           float value) const;

    /** Reconstitute a feature value.  This method will read the given value
        of the given feature from a binary store.

        \param store         the binary store to which the feature is read.
        \param feature       the feature for which we are to read the value.
        \param value         the value to be read

        The default implemention will, depending upon the feature info for
        the given feature, either reconstitute directly a float or
        reconstitute a string and parse it.
    */
    virtual void reconstitute(DB::Store_Reader & store,
                              const Feature & feature,
                              float & value) const;
    //@}


    /*************************************************************************/
    /* FEATURE SETS                                                          */
    /*************************************************************************/

    /** \name Methods to deal with a feature set

        These methods control how an entire feature set gets handled.  They
        are mainly here to allow an entire feature set to be loaded and saved
        for debugging purposes or for when the data collection process is long
        and the data needs to be saved in between collecting it and using it
        @{
    */

    /** Print a feature set.  This method serializes the given feature set as
        ASCII to the given string.  Note that it is almost certain that you
        will want to implement this in such a way that only one line is
        produced (there are no embedded newlines).

        \param fs            the feature set to be printed

        The default implementation simply writes it in the sparse feature
        format: the feature, then a colon, then its value, separated by
        spaces.
    */
    virtual std::string print(const Feature_Set & fs) const;
    
    /** Serialize an entire feature set.  Although the feature set itself
        could be serialized without this method, some feature spaces can
        store it more compactly than this.  (For example, the
        Dense_Feature_Space can simply list the values rather than attaching
        the features to them; the features are implicit in the feature space).

        \param store         the store to write to.
        \param fs            the feature set to write.

        Any errors in the process (invalid features, store errors, etc)
        should throw an exception.
    */
    virtual void serialize(DB::Store_Writer & store,
                           const Feature_Set & fs) const;

    /** Reconstitute an entire feature set.  The default implementation
        can parse the same format given by the serialize method.

        \param store         the store to read from.
        \param fs            the feature set to read into.  Note that this
                             parameter is a reference to a shared pointer
                             to a feature set.  The shared pointer will be
                             reset to point to a reconstituted feature set.

        Throws an exception on an invalid store or unreconstitutible data.
    */
    virtual void reconstitute(DB::Store_Reader & store,
                              boost::shared_ptr<Feature_Set> & fs) const;
    //@}


    /*************************************************************************/
    /* FEATURE SPACE                                                         */
    /*************************************************************************/

    /** \name Methods to deal with the feature space as a whole
        These methods operate over the whole feature space.  They are mainly
        concerned with making sure the feature space and its parameters can
        be correctly serialized, reconstituted and copied, to enable the
        persistence framework to do its job.

        @{
    */

    /** Return the ID of this class.
        For the purposes of polymorphic extraction, every subclass needs to
        return a unique class ID.  This method returns that ID.  If
        polymorphic extraction is not used it doesn't matter what this method
        returns; there is no default implementation to ensure that it is never
        forgotten to override it.

        \returns             the polymorphic ID of the class.  The name of its
                             type is a good thing to use.
     */
    virtual std::string class_id() const = 0;

    typedef Feature_Space_Type Type;

    /** Return the feature space type. */
    virtual Type type() const = 0;

    /** Return a dense set of features.  Default returns an empty list for
        sparse feature spaces and throws for dense feature spaces (which
        should override the method).
    */
    virtual const std::vector<Feature> & dense_features() const;

    /** Serialize the feature space.  This method is called to serialize the
        feature space.  For those where there is no need to do so, the default
        implementation will suffice which writes only the name of the store.

        \param store         the store to serialize to.

        Throws an exception on store errors.
    */
    virtual void serialize(DB::Store_Writer & store) const;

    /** Reconstitute the feature space.  The default implementation matches
        that of the serialize method: it attempts to read the class ID, and
        makes sure it matches that given by the class_id() method.

        \param store         the store to reconstitute from.
        \param fs            the feature space to model on.  Why is this here?

        An exception should be thrown on an error.
    */
    virtual void reconstitute(DB::Store_Reader & store,
                            const boost::shared_ptr<const Feature_Space> & fs);
    
    /** Return a copy of this feature space.  This needs to be overridden
        by each of the subclassed objects in the following manner:

        \code
        class FS_Subclass : public Feature_Space {
            ...

            virtual FS_Subclass * make_copy() const
            {
                return new FS_Subclass(*this);
            }

            ...
        };
        \endcode
    */
    virtual Feature_Space * make_copy() const = 0;

    /** Print the feature space.  This is used mainly to write the header of
        a data file.  The default implementation simply writes the class ID
        to the top of the file.
    */
    virtual std::string print() const;
    //@}

    /** Return a training data object compatible with this feature space.
        Default implementation constructs a new Training_Data instance.

        This method is used to enable training data files to be automatically
        without requiring a Training_Data object to be explicitly created.
        It is overridden by the Dense_Feature_Set to return an object of type
        Dense_Training_Data, as those two need to be used together.

        \param fs            feature space to give to the training data.  Will
                             normally be the same as *this, but we need to
                             pass it as a boost::shared_ptr<> so that the
                             reference counting will work.

    */
    virtual boost::shared_ptr<Training_Data>
    training_data(const boost::shared_ptr<const Feature_Space> & fs) const;

    /** Freeze the feature space.  This should cause all feature information
        to revert to a fixed form such that it can no longer be modified.
        Used to avoid string maps growing in an unbounded fashion.

        The default implementation does nothing.  This is not required to be
        implemented; it is a performance enhancement.
    */

    virtual void freeze();
};

/* Note the extra overloads necessary as the const shared pointers don't
   convert to the non-const ones. */
DB::Store_Writer &
operator << (DB::Store_Writer & store,
             const boost::shared_ptr<Feature_Space> & fs);

DB::Store_Writer &
operator << (DB::Store_Writer & store,
             const boost::shared_ptr<const Feature_Space> & fs);

DB::Store_Reader &
operator >> (DB::Store_Reader & store,
             boost::shared_ptr<Feature_Space> & fs);

DB::Store_Reader &
operator >> (DB::Store_Reader & store,
             boost::shared_ptr<const Feature_Space> & fs);


/*****************************************************************************/
/* MUTABLE_FEATURE_SPACE                                                     */
/*****************************************************************************/

/** A feature space that can be modified. */

class Mutable_Feature_Space : public Feature_Space {
public:
    
    virtual ~Mutable_Feature_Space();

    /** Set the information for how to use a feature.
        \param feature       The feature for which we are setting the
                             information.
        \param info          The new information.

        Note that feature spaces are not required to implement this method.
        The default implementation will throw an exception; any classes
        which do implement it (at least Dense_Feature_Space and
        Sparse_Feature_Space) need to override it.

        Throws an exception if the feature was not found.
    */
    virtual void set_info(const Feature & feature, const Feature_Info & info) = 0;

    /** Create a new feature with the given name and the given feature info.
        If one with the name already exists, it is returned instead and the
        info isn't set.
    */
    virtual Feature
    make_feature(const std::string & name,
                 const Feature_Info & info = UNKNOWN) = 0;

    /** Return the feature with the given name.  Throws if the name is
        unknown. */
    virtual Feature get_feature(const std::string & name) const = 0;

    /** Import another feature space.  This is capable of converting between
        different types of feature spaces; for example of converting sparse
        into dense or vice versa.
    */
    virtual void import(const Feature_Space & from);

    virtual Mutable_Feature_Space * make_copy() const = 0;
};


} // namespace ML

#endif /* __boosting__feature_space_h__ */
