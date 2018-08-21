//   SMCTC: sampler.hh
//
//   Copyright Adam Johansen, 2008.
//
//   This file is part of SMCTC.
//
//   SMCTC is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   SMCTC is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with SMCTC.  If not, see <http://www.gnu.org/licenses/>.

//! \file
//! \brief Defines the overall sampler object.
//!
//! This file defines the smc::sampler class which is used to implement entire particle systems.

#ifndef __SMC_SAMPLER_HH

#define __SMC_SAMPLER_HH 1.0

#include <algorithm>
#include <cstdlib>
#include <iosfwd>
#include <memory>
#include <utility>
#include <vector>

#ifdef SMCTC_HAVE_BGL
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/labeled_graph.hpp>
#include <boost/graph/graphviz.hpp>
#endif

#include "rng.hh"
#include "history.hh"
#include "moveset.hh"
#include "particle.hh"
#include "smc-exception.hh"

#if defined(_OPENMP)
#include <omp.h>
#endif

///Specifiers for various resampling algorithms:
enum ResampleType { SMC_RESAMPLE_MULTINOMIAL = 0,
                    SMC_RESAMPLE_RESIDUAL,
                    SMC_RESAMPLE_STRATIFIED,
                    SMC_RESAMPLE_SYSTEMATIC,
                    SMC_RESAMPLE_FRIBBLEBITS
                  };

///Storage types for the history of the particle system.
enum HistoryType { SMC_HISTORY_NONE = 0,
                   SMC_HISTORY_RAM
                 };

namespace smc
{

struct DatabaseHistory {
    std::vector<double> ess;

    void clear() {
        ess.clear();
    }
};

/// A template class for an interacting particle system suitable for SMC sampling
template <class Space>
class sampler
{
private:
    ///A random number generator.
    std::unique_ptr<rng> pRng;

    ///Number of particles in the system.
    long N;
    ///The current evolution time of the system.
    long T;

    ///The resampling mode which is to be employed.
    ResampleType rtResampleMode;
    ///The effective sample size at which resampling should be used.
    double dResampleThreshold;
    ///Structure used internally for resampling.
    std::vector<double> dRSWeights;
    ///Structure used internally for resampling.
    std::vector<unsigned int> uRSCount;
    ///Structure used internally for resampling.
    std::vector<unsigned int> uRSIndices;

    ///The particles within the system.
    std::vector<particle<Space>> pParticles;
    ///The set of moves available.
    moveset<Space> Moves;

    ///The number of MCMC moves which have been accepted during this iteration
    int nAccepted;
    ///A flag which tracks whether the ensemble was resampled during this iteration
    int nResampled;

    ///A mode flag which indicates whether historical information is stored
    HistoryType htHistoryMode;
    ///The historical process associated with the particle system.
    history<particle<Space> > History;
#if defined(_OPENMP)
	std::size_t nThreads;
#endif

#ifdef SMCTC_HAVE_BGL
    /// A vertex in the particle history graph:
    /// (generation, particle index)
    typedef std::pair<size_t, size_t> Vertex;
    /// Particle history graph
    typedef boost::labeled_graph <
    boost::adjacency_list< boost::vecS, boost::vecS, boost::directedS, size_t >,
          Vertex
          > Graph;

    Graph g;
#endif

public:
    ///Create an particle system containing lSize uninitialised particles with the specified mode.
    sampler(long lSize, HistoryType htHistoryMode);
    ///Create an particle system constaining lSize uninitialised particles with the specified mode and random number generator.
    sampler(long lSize, HistoryType htHistoryMode, const gsl_rng_type* rngType, unsigned long nSeed);
    ///Dispose of a sampler.
    ~sampler();
    ///Calculates and Returns the Effective Sample Size.
    double GetESS(void) const;
    ///Returns a pointer to the History of the particle system
    const history<particle<Space> > * GetHistory(void) const { return &History; }
    ///Returns the number of particles within the system.
    long GetNumber(void) const {return N;}
    ///Return the value of particle n
    const Space &  GetParticleValue(int n) { return pParticles[n].GetValue(); }
    ///Return the logarithmic unnormalized weight of particle n
    double GetParticleLogWeight(int n) { return pParticles[n].GetLogWeight(); }
    ///Return the unnormalized weight of particle n
    double GetParticleWeight(int n) { return pParticles[n].GetWeight(); }
    ///Returns the current evolution time of the system.
    long GetTime(void) const {return T;}
    ///Initialise the sampler and its constituent particles.
    void Initialise(void);
    ///Integrate the supplied function with respect to the current particle set.
    double Integrate(double(*pIntegrand)(const Space &, void*), void* pAuxiliary);
    ///Integrate the supplied function over the path path using the supplied width function.
    double IntegratePathSampling(double(*pIntegrand)(long, const particle<Space>&, void*), double(*pWidth)(long, void*), void* pAuxiliary);
    ///Perform one iteration of the simulation algorithm.
    void Iterate(void);
    ///Cancel one iteration of the simulation algorithm.
    void IterateBack(void);
    ///Perform one iteration of the simulation algorithm and return the resulting ess
    double IterateEss(void);

    double IterateEssVariable(DatabaseHistory* database_history=nullptr);

    ///Perform iterations until the specified evolution time is reached
    void IterateUntil(long lTerminate);
    ///Move the particle set by proposing an applying an appropriate move to each particle.
    void MoveParticles(void);
    ///Resample the particle set using the specified resmpling scheme.
    void Resample(ResampleType lMode);

    const std::vector<unsigned int> SampleMultinomial(long M) const;
    const std::vector<unsigned int> SampleSystematic(long M, bool bStratified=false) const;
    const std::vector<unsigned int> SampleStratified(long M) const;

    ///Resample the particle set using fribblebits resampling.
    void ResampleFribble(double dEss);
    ///Sets the entire moveset to the one which is supplied
    void SetMoveSet(moveset<Space>& pNewMoveset) { Moves = pNewMoveset; }
    ///Set Resampling Parameters
    void SetResampleParams(ResampleType rtMode, double dThreshold);
    ///Dump a specified particle to the specified output stream in a human readable form
    std::ostream & StreamParticle(std::ostream & os, long n);
    ///Dump the entire particle set to the specified output stream in a human readable form
    std::ostream & StreamParticles(std::ostream & os);
    ///Allow a human readable version of the sampler configuration to be produced using the stream operator.
    /// std::ostream & operator<< (std::ostream& os, sampler<Space> & s);

#ifdef SMCTC_HAVE_BGL
    /// Dump the particle graph to an output stream
    std::ostream & StreamParticleGraph(std::ostream & os) const;
#endif

	/// \brief Set the number of threads
	/// \param nThreads Number of threads
#if defined(_OPENMP)
	void SetNumberOfThreads(const size_t n)
	{ this->nThreads = n; };
#endif

private:
    ///Duplication of smc::sampler is not currently permitted.
    sampler(const sampler<Space> & sFrom);
    ///Duplication of smc::sampler is not currently permitted.
    sampler<Space> & operator=(const sampler<Space> & sFrom);

#ifdef SMCTC_HAVE_BGL
    /// Add a level to the particle history graph
    ///
    /// \param parents: length N array with the parent index of each new particle. Alternatively, \c nullptr may be
    /// passed to indicate that no resampling was performed.
    ///
    /// Should be called before incrementing T
    void UpdateParticleGraph(const unsigned int* parents);
#endif
};


/// The constructor prepares a sampler for use but does not assign any moves to the moveset, initialise the particles
/// or otherwise perform any sampling related tasks. Its main function is to allocate a region of memory in which to
/// store the particle set and to initialise a random number generator.
///
/// \param lSize The number of particles present in the ensemble (at time 0 if this is a variable quantity)
/// \param htHM The history mode to use: set this to SMC_HISTORY_RAM to store the whole history of the system and SMC_HISTORY_NONE to avoid doing so.
/// \tparam Space The class used to represent a point in the sample space.
template <class Space>
sampler<Space>::sampler(long lSize, HistoryType htHM) :
    pRng(new rng()),
    N(lSize)
{
    pParticles.resize(lSize);

    //Allocate some storage for internal workspaces
    dRSWeights.resize(N);
    ///Structure used internally for resampling.
    uRSCount.resize(N);
    ///Structure used internally for resampling.
    uRSIndices.resize(N);

    //Some workable defaults.
    htHistoryMode = htHM;
    rtResampleMode = SMC_RESAMPLE_STRATIFIED;
    dResampleThreshold = 0.5 * N;
#if defined(_OPENMP)
	nThreads = 1;
#endif
}

/// The constructor prepares a sampler for use but does not assign any moves to the moveset, initialise the particles
/// or otherwise perform any sampling related tasks. Its main function is to allocate a region of memory in which to
/// store the particle set and to initialise a random number generator.
///
/// \param lSize The number of particles present in the ensemble (at time 0 if this is a variable quantity)
/// \param htHM The history mode to use: set this to SMC_HISTORY_RAM to store the whole history of the system and SMC_HISTORY_NONE to avoid doing so.
/// \param rngType The type of random number generator to use
/// \param rngSeed The seed to use for the random number generator
/// \tparam Space The class used to represent a point in the sample space.
template <class Space>
sampler<Space>::sampler(long lSize, HistoryType htHM, const gsl_rng_type* rngType, unsigned long rngSeed) :
    pRng(new rng(rngType, rngSeed)),
    N(lSize)
{
    pParticles.resize(lSize);

    //Allocate some storage for internal workspaces
    dRSWeights.resize(N);
    ///Structure used internally for resampling.
    uRSCount.resize(N);
    ///Structure used internally for resampling.
    uRSIndices.resize(N);

    //Some workable defaults.
    htHistoryMode  = htHM;
    rtResampleMode = SMC_RESAMPLE_STRATIFIED;
    dResampleThreshold = 0.5 * N;
#if defined(_OPENMP)
	nThreads = 1;
#endif
}


template <class Space>
sampler<Space>::~sampler()
{
}

template <class Space>
double sampler<Space>::GetESS(void) const
{
    long double sum = 0;
    long double sumsq = 0;

    for(int i = 0; i < pParticles.size(); i++)
        sum += expl(pParticles[i].GetLogWeight());

    for(int i = 0; i < pParticles.size(); i++)
        sumsq += expl(2.0 * (pParticles[i].GetLogWeight()));

    return expl(-log(sumsq) + 2 * log(sum));
}

/// At present this function resets the system evolution time to 0 and calls the moveset initialisor to assign each
/// particle in the ensemble.
///
/// Note that the initialisation function must be specified before calling this function.
template <class Space>
void sampler<Space>::Initialise(void)
{
    T = 0;

    for(int i = 0; i < N; i++)
        pParticles[i] = Moves.DoInit(pRng.get());

    if(htHistoryMode != SMC_HISTORY_NONE) {
        while(History.Pop());
        nResampled = 0;
        History.Push(N, pParticles.data(), 0, historyflags(nResampled));
    }

    return;
}

/// This function returns the result of integrating the supplied function under the empirical measure associated with the
/// particle set at the present time. The final argument of the integrand function is a pointer which will be supplied
/// with pAuxiliary to allow for arbitrary additional information to be passed to the function being integrated.
///
/// \param pIntegrand The function to integrate with respect to the particle set
/// \param pAuxiliary A pointer to any auxiliary data which should be passed to the function

template <class Space>
double sampler<Space>::Integrate(double(*pIntegrand)(const Space&, void*), void * pAuxiliary)
{
    long double rValue = 0;
    long double wSum = 0;
    for(int i = 0; i < N; i++) {
        rValue += expl(pParticles[i].GetLogWeight()) * pIntegrand(pParticles[i].GetValue(), pAuxiliary);
        wSum  += expl(pParticles[i].GetLogWeight());
    }

    rValue /= wSum;
    return (double)rValue;
}

/// This function is intended to be used to estimate integrals of the sort which must be evaluated to determine the
/// normalising constant of a distribution obtain using a sequence of potential functions proportional to densities with respect
/// to the initial distribution to define a sequence of distributions leading up to the terminal, interesting distribution.
///
/// In this context, the particle set at each time is used to make an estimate of the path sampling integrand, and a
/// trapezoidal integration is then performed to obtain an estimate of the path sampling integral which is the natural logarithm
/// of the ratio of normalising densities.
///
/// \param pIntegrand  The quantity which we wish to integrate at each time
/// \param pWidth      A pointer to a function which specifies the width of each

template <class Space>
double sampler<Space>::IntegratePathSampling(double(*pIntegrand)(long, const particle<Space> &, void*), double(*pWidth)(long, void*), void* pAuxiliary)
{
    if(htHistoryMode == SMC_HISTORY_NONE)
        throw SMC_EXCEPTION(SMCX_MISSING_HISTORY, "The path sampling integral cannot be computed as the history of the system was not stored.");

    History.Push(N, pParticles.data(), nAccepted, historyflags(nResampled));
    double dRes = History.IntegratePathSampling(pIntegrand, pWidth, pAuxiliary);
    History.Pop();
    return dRes;
}

/// The iterate function:
///         -# appends the current particle set to the history if desired
///          -# moves the current particle set
///         -# checks the effective sample size and resamples if necessary
///         -# performs a mcmc step if required
///         -# increments the current evolution time
template <class Space>
void sampler<Space>::Iterate(void)
{
    IterateEss();
    return;
}

template <class Space>
void sampler<Space>::IterateBack(void)
{
    if(htHistoryMode == SMC_HISTORY_NONE)
        throw SMC_EXCEPTION(SMCX_MISSING_HISTORY, "An attempt to undo an iteration was made; unforunately, the system history has not been stored.");

    History.Pop(&N, &pParticles, &nAccepted, NULL);
    T--;
    return;
}

template <class Space>
const std::vector<unsigned int> sampler<Space>::SampleMultinomial(long M) const
{
    // Collect the weights of the particles.
    std::vector<double> dWeights(pParticles.size());
    for (size_t i = 0; i < pParticles.size(); ++i) {
        dWeights[i] = pParticles[i].GetWeight();
    }

    // Generate a vector of sample counts.
    std::vector<unsigned int> uCount(pParticles.size());
    pRng->Multinomial(M, pParticles.size(), dWeights.data(), uCount.data());

    // Transform the vector of sample counts into a vector of parent indices.
    std::vector<unsigned int> uIndices(M);
    for (size_t i = 0, j = 0; i < uCount.size(); ++i) {
        while (uCount[i] > 0) {
            uIndices[j++] = i;
            --uCount[i];
        }
    }

    return uIndices;
}

template <class Space>
const std::vector<unsigned int> sampler<Space>::SampleSystematic(long M, bool bStratified) const
{
    // Procedure for stratified sampling
    // See Appendix of Kitagawa 1996, http://www.jstor.org/stable/1390750,
    // or p.290 of the Doucet et al book, an image of which is at:
    // http://cl.ly/image/200T0y473k1d/stratified_resampling.jpg

    // Calculate the normalising constant of the weight vector
    double dWeightSum = 0.0;
    for (size_t i = 0; i < pParticles.size(); ++i)
        dWeightSum += exp(pParticles[i].GetLogWeight());

    // dWeightCumulative is \tilde{\pi}^r from the Doucet book.
    double dWeightCumulative = 0.0;
    //Generate a uniform random number between 0 and 1/M.
    double dRand = pRng->Uniform(0, 1.0 / M);

    std::vector<unsigned int> uCount(pParticles.size(), 0);

    for (size_t i = 0, j = 0; i < pParticles.size() && j < M; ++i) {
        dWeightCumulative += exp(pParticles[i].GetLogWeight()) / dWeightSum;
        while (dWeightCumulative > ((static_cast<double>(j) / M) + dRand) && j < M) {
            ++uCount[i];
            ++j;

            // The only difference between stratified and systematic resampling
            // is whether a new random variable is drawn for each partition of
            // the (0, 1] interval.
            if (bStratified) {
                dRand = pRng->Uniform(0, 1.0 / M);
            }
        }
    }

    // Transform the vector of sample counts into a vector of parent indices.
    std::vector<unsigned int> uIndices(M);
    for (size_t i = 0, j = 0; i < uCount.size(); ++i) {
        while (uCount[i] > 0) {
            uIndices[j++] = i;
            --uCount[i];
        }
    }

    return uIndices;
}

template <class Space>
const std::vector<unsigned int> sampler<Space>::SampleStratified(long M) const
{
    return SampleSystematic(M, true);
}

template <class Space>
void sampler<Space>::ResampleFribble(double dESS)
{
    assert(pParticles.size() == N);

    //
    // Generate new samples until ESS reaches the threshold.
    //

    std::clog << "[ResampleFribble] starting ESS = " << dESS << '\n';

    while (dESS < dResampleThreshold) {
        //long M = static_cast<long>(std::ceil(dResampleThreshold - dESS));
        long M = N;

        // Select M parents from the current population.
        const auto uIndices = SampleStratified(M);

        // Generate M new particles by perturbation of the selected parents.
        pParticles.reserve(pParticles.size() + M);
        for (size_t i = 0; i < uIndices.size(); ++i) {
            const auto& parent = pParticles[uIndices[i]];
            pParticles.emplace_back(parent.GetValue(), parent.GetLogWeight());
            Moves.DoMCMC(T + 1, pParticles.back(), pRng.get());
        }

        dESS = GetESS();

        std::clog << "[ResampleFribble] generated " << M << " new particles\n";
        std::clog << "[ResampleFribble] new ESS = " << dESS << '\n';
    }

    //
    // Resample the population back down to N particles.
    //

    std::clog << "[ResampleFribble] downsampling from " << pParticles.size() << " to " << N << " particles\n";

    const auto uIndices = SampleStratified(N);
    decltype(pParticles) pNewParticles;
    pNewParticles.reserve(N);

    // Replicate the chosen particles.
    for (size_t i = 0; i < uIndices.size() ; ++i) {
        const auto& parent = pParticles[uIndices[i]];
        pNewParticles.emplace_back(parent.GetValue(), 0.0);
    }

    pParticles = pNewParticles;
    assert(pParticles.size() == N);
}

template <class Space>
double sampler<Space>::IterateEssVariable(DatabaseHistory* database_history)
{
    assert(pParticles.size() == N);

    // Append the current population to the history, if requested.
    if (htHistoryMode != SMC_HISTORY_NONE)
        History.Push(N, pParticles.data(), nAccepted, historyflags(nResampled));

    // Stash copies of the original particles; we'll need them to generate new ones.
    const auto pStartingParticles = pParticles;
    pParticles.clear();

    double dESS = 0.0;
    double dGlobalMaxWeight = -std::numeric_limits<double>::infinity();

    if (database_history)
        database_history->clear();

    do {
        // Generate new particles from the originals via SMC moves.
        auto pNewParticles = pStartingParticles;
		#pragma omp parallel for num_threads(nThreads)
		for(int i = 0; i < N; i++) {
            Moves.DoMove(T + 1, pNewParticles[i], pRng.get());
        }

        // Normalize the weights.
        double dLocalMaxWeight = -std::numeric_limits<double>::infinity();
        for (const auto& p : pNewParticles)
            dLocalMaxWeight = std::max(dLocalMaxWeight, p.GetLogWeight());

        //
        // TODO: Clean up this spaghetti.
        //

        if (pParticles.empty())
            dGlobalMaxWeight = dLocalMaxWeight;

        if (dLocalMaxWeight > dGlobalMaxWeight) {
            for (auto& p : pParticles)
                p.AddToLogWeight(dGlobalMaxWeight - dLocalMaxWeight);
            for (auto& p : pNewParticles)
                p.AddToLogWeight(-dLocalMaxWeight);

            dGlobalMaxWeight = dLocalMaxWeight;
        } else {
            for (auto& p : pNewParticles)
                p.AddToLogWeight(-dGlobalMaxWeight);
        }

        // Add the newly-generated particles to the population.
        pParticles.insert(pParticles.end(), pNewParticles.begin(), pNewParticles.end());

        dESS = GetESS();
        std::clog << "[IterateEssVariable] ESS = " << dESS << ", N = " << pParticles.size() << '\n';

        if (database_history) {
            database_history->ess.push_back(dESS);
        }
    } while (dESS < dResampleThreshold && pParticles.size() < 100000);

    //
    // Resample the population back down to N particles.
    //

    if (pParticles.size() > N) {
        nResampled = 1;

        std::clog << "[IterateEssVariable] downsampling from " << pParticles.size() << " to " << N << " particles\n";

        auto uIndices = SampleStratified(N);
        decltype(pParticles) pSampledParticles;
        pSampledParticles.reserve(N);

        // Replicate the chosen particles.
        for (size_t i = 0; i < uIndices.size() ; ++i) {
            const auto& parent = pParticles[uIndices[i]];
            pSampledParticles.emplace_back(parent.GetValue(), 0.0);
        }

        pParticles = pSampledParticles;
    }

    //
    // (Optional) MCMC moves.
    //

    double nAcceptedLocal = 0;
	#pragma omp parallel for reduction(+:nAcceptedLocal) num_threads(nThreads)
	for(int i = 0; i < N; i++) {
		if(Moves.DoMCMC(T + 1, pParticles[i], pRng.get()))
            ++nAcceptedLocal;
    }
	nAccepted = nAcceptedLocal;
    ++T;

    assert(pParticles.size() == N);
    return dESS;
}

template <class Space>
double sampler<Space>::IterateEss(void)
{
    //Initially, the current particle set should be appended to the historical process.
    if(htHistoryMode != SMC_HISTORY_NONE)
        History.Push(N, pParticles.data(), nAccepted, historyflags(nResampled));

    nAccepted = 0;

    //Move the particle set.
    MoveParticles();

    //Normalise the weights to sensible values....
    double dMaxWeight = -std::numeric_limits<double>::infinity();
    for(int i = 0; i < N; i++)
        dMaxWeight = std::max(dMaxWeight, pParticles[i].GetLogWeight());
    for(int i = 0; i < N; i++)
        pParticles[i].SetLogWeight(pParticles[i].GetLogWeight() - (dMaxWeight));


    //Check if the ESS is below some reasonable threshold and resample if necessary.
    //A mechanism for setting this threshold is required.
    double ESS = GetESS();
    if(ESS < dResampleThreshold) {
        nResampled = 1;
        if (rtResampleMode == SMC_RESAMPLE_FRIBBLEBITS) {
            ResampleFribble(ESS);
        } else {
            Resample(rtResampleMode);
        }
    } else {
#ifdef SMCTC_HAVE_BGL
        UpdateParticleGraph(nullptr);
#endif
        nResampled = 0;
    }

    if (rtResampleMode != SMC_RESAMPLE_FRIBBLEBITS) {
		double nAcceptedLocal = nAccepted;
        //A possible MCMC step should be included here.
		#pragma omp parallel for reduction(+:nAcceptedLocal) num_threads(nThreads)
        for(int i = 0; i < N; i++) {
            if(Moves.DoMCMC(T + 1, pParticles[i], pRng.get()))
                nAcceptedLocal++;
        }
		nAccepted = nAcceptedLocal;
    }

    // Increment the evolution time.
    T++;

    return ESS;
}

template <class Space>
void sampler<Space>::IterateUntil(long lTerminate)
{
    while(T < lTerminate)
        Iterate();
}

template <class Space>
void sampler<Space>::MoveParticles(void)
{
	#pragma omp parallel for num_threads(nThreads)
    for(int i = 0; i < N; i++) {
        Moves.DoMove(T + 1, pParticles[i], pRng.get());
        //  pParticles[i].Set(pNew.value, pNew.logweight);
    }
}

///Perform resampling.
///Note: this procedure sets all particle weights to zero after resampling.
///\param lMode The sampling mode for the sampler.
template <class Space>
void sampler<Space>::Resample(ResampleType lMode)
{
    //Resampling is done in place.
    double dWeightSum = 0;
    unsigned uMultinomialCount;

    //First obtain a count of the number of children each particle has via the chosen strategy.
    //This will be stored in uRSCount.
    switch(lMode) {
    case SMC_RESAMPLE_MULTINOMIAL:
        //Sample from a suitable multinomial vector
        for(int i = 0; i < N; ++i)
            dRSWeights[i] = pParticles[i].GetWeight();
        //Generate a multinomial random vector with parameters (N,dRSWeights[1:N]) and store it in uRSCount
        pRng->Multinomial(N, N, dRSWeights.data(), uRSCount.data());
        break;

    case SMC_RESAMPLE_RESIDUAL:
        //Sample from a suitable multinomial vector and add the integer replicate
        //counts afterwards.
        dWeightSum = 0;
        for(int i = 0; i < N; ++i) {
            dRSWeights[i] = pParticles[i].GetWeight();
            dWeightSum += dRSWeights[i];
        }

        uMultinomialCount = N;
        for(int i = 0; i < N; ++i) {
            dRSWeights[i] = N * dRSWeights[i] / dWeightSum;
            uRSIndices[i] = unsigned(floor(dRSWeights[i])); //Reuse temporary storage.
            dRSWeights[i] = (dRSWeights[i] - uRSIndices[i]);
            uMultinomialCount -= uRSIndices[i];
        }
        //Generate a multinomial random vector with parameters (uMultinomialCount,dRSWeights[1:N]) and store it in uRSCount
        pRng->Multinomial(uMultinomialCount, N, dRSWeights.data(), uRSCount.data());
        for(int i = 0; i < N; ++i)
            uRSCount[i] += uRSIndices[i];
        break;


    case SMC_RESAMPLE_STRATIFIED:
    default: {
        // Procedure for stratified sampling
        // See Appendix of Kitagawa 1996, http://www.jstor.org/stable/1390750,
        // or p.290 of the Doucet et al book, an image of which is at:
        // http://cl.ly/image/200T0y473k1d/stratified_resampling.jpg
        dWeightSum = 0;
        double dWeightCumulative = 0;
        // Calculate the normalising constant of the weight vector
        for(int i = 0; i < N; i++)
            dWeightSum += exp(pParticles[i].GetLogWeight());
        //Generate a random number between 0 and 1/N.
        double dRand = pRng->Uniform(0, 1.0 / ((double)N));
        // Clear out uRSCount.
        for(int i = 0; i < N; ++i)
            uRSCount[i] = 0;
        // 0 <= j < N will index the current sampling step, whereas k will index the previous step.
        int j = 0, k = 0;
        // dWeightCumulative is \tilde{\pi}^r from the Doucet book.
        dWeightCumulative = exp(pParticles[0].GetLogWeight()) / dWeightSum;
        // Advance j along until dWeightCumulative > j/N + dRand
        while(j < N) {
            while((dWeightCumulative - dRand) > ((double)j) / ((double)N) && j < N) {
                uRSCount[k]++; // Accept the particle k.
                j++;
                dRand = pRng->Uniform(0, 1.0 / ((double)N));
            }
            k++;
            dWeightCumulative += exp(pParticles[k].GetLogWeight()) / dWeightSum;
        }
        break;
    }

    case SMC_RESAMPLE_SYSTEMATIC: {
        // Procedure for stratified sampling but with a common RV for each stratum
        dWeightSum = 0;
        double dWeightCumulative = 0;
        // Calculate the normalising constant of the weight vector
        for(int i = 0; i < N; i++)
            dWeightSum += exp(pParticles[i].GetLogWeight());
        //Generate a random number between 0 and 1/N times the sum of the weights
        double dRand = pRng->Uniform(0, 1.0 / ((double)N));

        int j = 0, k = 0;
        for(int i = 0; i < N; ++i)
            uRSCount[i] = 0;

        dWeightCumulative = exp(pParticles[0].GetLogWeight()) / dWeightSum;
        while(j < N) {
            while((dWeightCumulative - dRand) > ((double)j) / ((double)N) && j < N) {
                uRSCount[k]++;
                j++;

            }
            k++;
            dWeightCumulative += exp(pParticles[k].GetLogWeight()) / dWeightSum;
        }
        break;

    }
    }

    //Map count to indices to allow in-place resampling.
    //Here j represents the next particle from the previous generation that is going to be dropped in the current
    //sample, and thus can get filled by the resampling scheme.
    for(unsigned int i = 0, j = 0; i < N; ++i) {
        if(uRSCount[i] > 0) {
            uRSIndices[i] = i;
            while(uRSCount[i] > 1) {
                while(uRSCount[j] > 0) ++j; // find next free spot
                uRSIndices[j++] = i; // assign index
                --uRSCount[i]; // decrement number of remaining offsprings
            }
        }
    }

#ifdef SMCTC_HAVE_BGL
    UpdateParticleGraph(uRSIndices.data());
#endif

    //Perform the replication of the chosen.
    for(unsigned int i = 0; i < N ; ++i) {
        if(uRSIndices[i] != i)
            pParticles[i].SetValue(pParticles[uRSIndices[i]].GetValue());
        //Reset the log weight of the particles to be zero.
        pParticles[i].SetLogWeight(0);
    }
}

/// This function configures the resampling parameters, allowing the specification of both the resampling
/// mode and the threshold at which resampling is used.
///
/// \param rtMode The resampling mode to be used.
/// \param dThreshold The threshold at which resampling is deemed necesary.
///
/// The rtMode parameter should be set to one of the following:
/// -# SMC_RESAMPLE_MULTINOMIAL to use multinomial resampling
/// -# SMC_RESAMPLE_RESIDUAL to use residual resampling
/// -# SMC_RESAMPLE_STRATIFIED to use stratified resampling
/// -# SMC_RESAMPLE_SYSTEMATIC to use systematic resampling
///
/// The dThreshold parameter can be set to a value in the range [0,1) corresponding to a fraction of the size of
/// the particle set or it may be set to an integer corresponding to an actual effective sample size.

template <class Space>
void sampler<Space>::SetResampleParams(ResampleType rtMode, double dThreshold)
{
    rtResampleMode = rtMode;
    if(dThreshold < 1)
        dResampleThreshold = dThreshold * N;
    else
        dResampleThreshold = dThreshold;
}

template <class Space>
std::ostream & sampler<Space>::StreamParticle(std::ostream & os, long n)
{
    os << pParticles[n] << std::endl;
    return os;
}

template <class Space>
std::ostream & sampler<Space>::StreamParticles(std::ostream & os)
{
    for(int i = 0; i < N - 1; i++)
        os << pParticles[i] << std::endl;
    os << pParticles[N - 1] << std::endl;

    return os;
}

#ifdef SMCTC_HAVE_BGL
template <class Space>
std::ostream & sampler<Space>::StreamParticleGraph(std::ostream & os) const
{
    boost::write_graphviz(os, this->g);
    return os;
}

template <class Space>
void sampler<Space>::UpdateParticleGraph(const unsigned int* parents)
{
    const Vertex root(0, 0);
    if(T == 0) {
        // Initial particles - add each to the graph with no parent.
        for(size_t i = 0; i < N; ++i)
            boost::add_vertex(Vertex(T + 1, i), g);
    } else {
        // Add an edge between each particle and its parent
        for(size_t i = 0; i < N; ++i) {
            const Vertex u = T == 0 ? root : Vertex(T, parents == nullptr ? i : parents[i]);
            const Vertex v = Vertex(T + 1, i);
            assert(boost::vertex_by_label(u, g) != boost::graph_traits<Graph>::null_vertex() &&
                   "Cannot find vertex!");
            boost::add_vertex(v, g);
            boost::add_edge_by_label(u, v, g);
        }
    }
}
#endif


}

namespace std
{
/// Produce a human-readable display of the state of an smc::sampler class using the stream operator.

/// \param os The output stream to which the display should be made.
/// \param s  The sampler which is to be displayed.
template <class Space>
std::ostream & operator<< (std::ostream & os, smc::sampler<Space> & s)
{
    os << "Sampler Configuration:" << std::endl;
    os << "======================" << std::endl;
    os << "Evolution Time:   " << s.GetTime() << std::endl;
    os << "Particle Set Size:" << s.GetNumber() << std::endl;
    os << std::endl;
    os << "Particle Set:" << std::endl;
    s.StreamParticles(os);
    os << std::endl;
    return os;
}
}
#endif
