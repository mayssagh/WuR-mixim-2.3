

#ifndef ICONTROL_H_
#define ICONTROL_H_

/**
 * @brief Abstract interface for controllable components.
 */
class IControl {
public:
    enum Controls {POWER_ON_PCA=100, POWER_OFF_PCA, POWER_ON_CSMA, POWER_OFF_CSMA};
    enum Status {AWAKE_PCA=200, SLEEPING_PCA, AWAKE_CSMA, SLEEPING_CSMA };


    virtual ~IControl() {};
    virtual bool isAwakePCA() {return status == AWAKE_PCA;};
    virtual bool isSleepingPCA() {return status == SLEEPING_PCA;};
    virtual Status getStatus() const {return status;}
    virtual Status getStatusCSMA() const {return statusCSMA;}
    virtual bool isAwakeCsma() {return statusCSMA == AWAKE_CSMA;};
    virtual bool isSleepingCSMA() {return statusCSMA == SLEEPING_CSMA;};



protected:

    virtual bool powerOnPCA() = 0;
    virtual bool powerOffPCA() = 0;
    virtual bool powerOnCSMA() = 0;
    virtual bool powerOffCSMA() = 0;

    Status status;
    Status statusCSMA;


};

#endif /* ICONTROL_H_ */
