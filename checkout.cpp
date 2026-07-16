#include <iostream>
#include <vector>
#include <string>
using namespace std;

//starting with the payment gateway

class PaymentProvider
{
public:
    virtual bool charge(int amount) = 0;
    virtual void rollback(int amount) = 0;

    virtual bool supportsRefund() = 0;
    virtual bool supportsPartialCapture() = 0;
    virtual bool isIdempotent() = 0;

    virtual string getName() = 0;

    virtual ~PaymentProvider() {}
};

//the Stripe logic

class Stripe : public PaymentProvider
{
public:
    bool charge(int amount)
    {
        cout << "Stripe charged $" << amount << endl;
        return true;
    }

    void rollback(int amount)
    {
        cout << "Stripe refunded $" << amount << endl;
    }

    bool supportsRefund()
    {
        return true;
    }

    bool supportsPartialCapture()
    {
        return true;
    }

    bool isIdempotent()
    {
        return true;
    }

    string getName()
    {
        return "Stripe";
    }
};

//the paypal logic

class Paypal : public PaymentProvider
{
public:
    bool charge(int amount)
    {
        cout << "Paypal charged $" << amount << endl;

        // simulate failure
        if(amount > 60)
            return false;

        return true;
    }

    void rollback(int amount)
    {
        cout << "Paypal refunded $" << amount << endl;
    }

    bool supportsRefund()
    {
        return true;
    }

    bool supportsPartialCapture()
    {
        return false;
    }

    bool isIdempotent()
    {
        return true;
    }

    string getName()
    {
        return "Paypal";
    }
};

//if this is a wallet

class StoreCredit : public PaymentProvider
{
public:
    bool charge(int amount)
    {
        cout << "Wallet deducted $" << amount << endl;
        return true;
    }

    void rollback(int amount)
    {
        cout << "Wallet restored $" << amount << endl;
    }

    bool supportsRefund()
    {
        return true;
    }

    bool supportsPartialCapture()
    {
        return false;
    }

    bool isIdempotent()
    {
        return false;
    }

    string getName()
    {
        return "Store Credit";
    }
};

//the 2nd interface {fraud check}

class FraudRule
{
public:
    virtual bool check() = 0;

    virtual ~FraudRule() {}
};

//suppose its a fraud normal one

class BasicFraud : public FraudRule
{
public:
    bool check()
    {
        cout << "Basic Fraud Check Passed" << endl;
        return true;
    }
};

//payment interfce

class PaymentOrchestrator
{
    vector<PaymentProvider*> providers;

public:

    void addProvider(PaymentProvider* provider)
    {
        providers.push_back(provider);
    }

    bool process(vector<int> amounts)
    {
        vector<pair<PaymentProvider*,int>> success;

        for(int i=0;i<providers.size();i++)
        {
            PaymentProvider* p = providers[i];

            cout << "\nCharging through "
                 << p->getName() << endl;

            bool paymentSuccess = p->charge(amounts[i]);

            if(paymentSuccess)
            {
                success.push_back({p,amounts[i]});
            }
            else
            {
                cout << "\nPayment Failed!" << endl;

                cout << "Rolling Back..." << endl;

                for(int j=success.size()-1;j>=0;j--)
                {
                    success[j].first->rollback(success[j].second);
                }

                return false;
            }
        }

        return true;
    }
};

//the checkout service 

class CheckoutService
{
    vector<FraudRule*> rules;

    PaymentOrchestrator payment;

public:

    void addFraudRule(FraudRule* rule)
    {
        rules.push_back(rule);
    }

    void addPaymentProvider(PaymentProvider* provider)
    {
        payment.addProvider(provider);
    }

    void checkout(vector<int> amounts)
    {
        cout<<"========== CHECKOUT ==========\n"<<endl;

        cout<<"Running Fraud Checks...\n"<<endl;

        for(auto rule : rules)
        {
            if(!rule->check())
            {
                cout<<"Fraud Detected"<<endl;
                return;
            }
        }

        cout<<"\nFraud Checks Completed\n"<<endl;

        bool success = payment.process(amounts);

        if(success)
        {
            cout<<"\nOrder Placed Successfully"<<endl;
        }
        else
        {
            cout<<"\nCheckout Failed"<<endl;
        }
    }
};

//if payment fails check for retry

void retry(PaymentProvider* provider,int amount)
{
    cout<<"\nRetry Logic"<<endl;

    if(provider->isIdempotent())
    {
        cout<<"Safe Retry for "
            <<provider->getName()<<endl;

        provider->charge(amount);
    }
    else
    {
        cout<<"Retry skipped for "
            <<provider->getName()
            <<" (Non-idempotent)"<<endl;
    }
}



int main()
{
    CheckoutService checkout;

    //fraud

    checkout.addFraudRule(new BasicFraud());

    //payment

    checkout.addPaymentProvider(new StoreCredit());

    checkout.addPaymentProvider(new Paypal());

    // Split Payment
    // Wallet = 30
    // Paypal = 70

    checkout.checkout({30,70});

    cout<<"\n----------\n";

    Stripe stripe;

    retry(&stripe,100);

    StoreCredit wallet;

    retry(&wallet,50);

    return 0;
}