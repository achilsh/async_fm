/**
 * @file: singleton.h
 * @brief: 
 * @author:  wusheng Hu
 * @version: v0x0001
 * @date: 2018-01-20
 */
#ifndef __SINGLETON_DEFINE_H_
#define __SINGLETON_DEFINE_H_

#ifdef __cplusplus 
    extern "C" {
#endif 

    #include <pthread.h>
    #include <stdlib.h>

#ifdef __cplusplus 
    }
#endif

//--------- define singleton using handler, __CLASSNAME is actual class name  ---------------------------//

#define SINGLETON(__CLASSNAME)  loss::CSingleton<__CLASSNAME>::Instance()

//------------------------------------------------------------------------//
namespace loss 
{
    template <typename T>
    class CSingleton
    {
     public:
      static T* Instance()  // or use volatile
      {
          if (!m_pInstance) 
          {
              pthread_mutex_lock(&m_mutex);
              if (!m_pInstance)
              {
                  if (m_Destoryed)
                  {
                      onDeadReference();
                  } 
                  else 
                  {
                      CreateInstance();
                  }
              }
              pthread_mutex_unlock(&m_mutex);
          }
          return m_pInstance;
      }

     private:
      static T* volatile m_pInstance; 
      static pthread_mutex_t m_mutex;
      static bool m_Destoryed;

     private:
      CSingleton();
      CSingleton(const CSingleton&);
      CSingleton& operator =(const  CSingleton&);
      ~CSingleton() 
      {
          m_pInstance = NULL;
          m_Destoryed = true;
      }

      static void CreateInstance() 
      {
          static T sInstance;
          m_pInstance = &sInstance;
      }

      static void onDeadReference() 
      {
          CreateInstance();
          new (m_pInstance) T;
          atexit(ExitInstance);
          m_Destoryed = false;
      }
      static void ExitInstance() 
      {
          m_pInstance->~T();
      }
    };

    /*** implement of interface ******/
    template<typename T>
    T* volatile CSingleton<T>::m_pInstance = NULL;
    
    template<typename T>
    pthread_mutex_t CSingleton<T>::m_mutex = PTHREAD_MUTEX_INITIALIZER;

    template<typename T>
    bool CSingleton<T>::m_Destoryed = false;
}

#endif

